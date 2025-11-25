#include "rconsole.h"
#include "rprocess.h"
#include <QKeyEvent>
#include <QTextCursor>
#include <QScrollBar>
#include <QFont>
#include <QRegularExpression>

RConsole::RConsole(RProcess *rProcess, QWidget *parent)
    : QTextEdit(parent)
    , rProcess(rProcess)
    , promptPosition(0)
    , isHidingOutput(false)
{
    setReadOnly(false);
    
    // Get current theme
    currentTheme = ThemeManager::instance().currentTheme();
    
    // Set font - try Hack first, fallback to monospace
    QFont font;
    QStringList fonts = {"Hack", "Noto Sans Mono", "Courier New", "Monospace"};
    for (const QString &fontName : fonts) {
        font = QFont(fontName, 10);
        if (QFontInfo(font).family() == fontName) {
            break;
        }
    }
    font.setStyleHint(QFont::TypeWriter);
    setFont(font);
    
    // Apply theme colors
    setStyleSheet(ThemeManager::instance().toStyleSheet(currentTheme));
    
    // Connect to R process signals
    if (rProcess) {
        connect(rProcess, &RProcess::outputReceived,
                this, &RConsole::onOutputReceived);
        connect(rProcess, &RProcess::errorReceived,
                this, &RConsole::onErrorReceived);
    }
    
    // Don't append prompt here, wait for R to start and send prompt
    // appendPrompt();
}

void RConsole::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        executeCurrentCommand();
        return;
    }
    
    // Prevent editing before prompt
    QTextCursor cursor = textCursor();
    if (cursor.position() < promptPosition && 
        event->key() != Qt::Key_Right &&
        event->key() != Qt::Key_Down &&
        event->key() != Qt::Key_End) {
        moveCursor(QTextCursor::End);
        return;
    }
    
    // Handle backspace at prompt position
    if (event->key() == Qt::Key_Backspace && cursor.position() <= promptPosition) {
        return;
    }
    
    QTextEdit::keyPressEvent(event);
}

void RConsole::onOutputReceived(const QString &output)
{
    // Check for form feed (clear console)
    if (output.contains('\f')) {
        clear();
        visibleBuffer.clear();
        // If there's content after form feed, process it
        int index = output.lastIndexOf('\f');
        if (index < output.length() - 1) {
            QString remaining = output.mid(index + 1);
            onOutputReceived(remaining);
        } else {
            appendPrompt();
        }
        return;
    }

    // Strip ANSI escape codes
    static QRegularExpression ansiRegex("\x1b\\[[0-9;]*[a-zA-Z]");
    QString cleanOutput = output;
    cleanOutput.replace(ansiRegex, "");

    // Append to visible buffer
    visibleBuffer.append(cleanOutput);
    
    // Process buffer if we have a newline or prompt
    bool hasNewline = visibleBuffer.contains('\n');
    bool hasPrompt = visibleBuffer.endsWith("> ") || visibleBuffer.endsWith(">");
    
    if (!hasNewline && !hasPrompt) {
        return; // Wait for more data
    }
    
    QString textToShow = visibleBuffer;
    visibleBuffer.clear();

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    
    // Set color for output
    QTextCharFormat format;
    format.setForeground(currentTheme.foreground);
    cursor.setCharFormat(format);
    
    // Strip trailing prompts from R output to avoid duplication
    // R often sends "> " at the end
    bool hadPrompt = false;
    while (textToShow.endsWith("> ") || textToShow.endsWith(">")) {
        textToShow.chop(textToShow.endsWith("> ") ? 2 : 1);
        hadPrompt = true;
    }
    
    // Try to strip command echo
    if (!lastCommand.isEmpty()) {
        QString echoStr = "> " + lastCommand;
        // Check if textToShow starts with echoStr (ignoring leading newlines)
        QString trimmedText = textToShow;
        while (trimmedText.startsWith("\n")) trimmedText = trimmedText.mid(1);
        
        if (trimmedText.startsWith(echoStr)) {
            textToShow = trimmedText.mid(echoStr.length());
            if (textToShow.startsWith("\n")) {
                textToShow = textToShow.mid(1);
            }
            lastCommand.clear(); // Only strip once per command
        } else if (trimmedText.startsWith(lastCommand)) {
             // Sometimes prompt is missing in echo?
            textToShow = trimmedText.mid(lastCommand.length());
            if (textToShow.startsWith("\n")) {
                textToShow = textToShow.mid(1);
            }
            lastCommand.clear();
        }
    }
    
    if (!textToShow.isEmpty()) {
        cursor.insertText(textToShow);
        if (!textToShow.endsWith('\n') && !textToShow.isEmpty()) {
            cursor.insertText("\n");
        }
    }
    
    setTextCursor(cursor);
    ensureCursorVisible();
    
    if (hadPrompt) {
        appendPrompt();
    }
}

void RConsole::onErrorReceived(const QString &error)
{
    // Strip ANSI escape codes
    static QRegularExpression ansiRegex("\x1b\\[[0-9;]*[a-zA-Z]");
    QString cleanError = error;
    cleanError.replace(ansiRegex, "");

    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    
    // Set color for errors (use keyword color for emphasis)
    QTextCharFormat format;
    format.setForeground(currentTheme.keyword);
    cursor.setCharFormat(format);
    
    cursor.insertText(cleanError);
    if (!cleanError.endsWith('\n')) {
        cursor.insertText("\n");
    }
    
    setTextCursor(cursor);
    ensureCursorVisible();
}

void RConsole::appendPrompt()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    
    // Set color for prompt
    QTextCharFormat format;
    format.setForeground(currentTheme.function);
    format.setFontWeight(QFont::Bold);
    cursor.setCharFormat(format);
    
    cursor.insertText("> ");
    
    // Reset format for user input
    format.setForeground(currentTheme.foreground);
    format.setFontWeight(QFont::Normal);
    cursor.setCharFormat(format);
    
    promptPosition = cursor.position();
    setTextCursor(cursor);
    ensureCursorVisible();
}

void RConsole::setTheme(const EditorTheme &theme)
{
    currentTheme = theme;
    setStyleSheet(ThemeManager::instance().toStyleSheet(theme));
}

void RConsole::executeCurrentCommand()
{
    QString command = getCurrentCommand();
    
    if (command.trimmed().isEmpty()) {
        append("");
        appendPrompt();
        return;
    }
    
    // Move cursor to end and add newline
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText("\n");
    setTextCursor(cursor);
    
    lastCommand = command.trimmed();
    
    // Execute command
    if (rProcess && rProcess->isRunning()) {
        rProcess->executeCommand(command);
    } else {
        onErrorReceived("R process is not running.");
        appendPrompt();
    }
}

QString RConsole::getCurrentCommand() const
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    
    // Get text from prompt position to end
    QString text = toPlainText();
    if (promptPosition < text.length()) {
        return text.mid(promptPosition);
    }
    
    return QString();
}
