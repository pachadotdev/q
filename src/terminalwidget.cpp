#include "terminalwidget.h"
#include <QDir>
#include <QFileInfo>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTextStream>
#include <QRegularExpression>

TerminalWidget::TerminalWidget(const QString &shell, QWidget *parent)
    : QTermWidget(0, parent)
    , shellPath(shell)
{
    // Get current theme
    currentTheme = ThemeManager::instance().currentTheme();
    
    // Set font to Hack Nerd Font Mono for Powerline/icon support
    // Note: qtermwidget may show a "variable-width font" warning due to how it checks
    // font metrics with Nerd Fonts' extra glyphs, but the font works correctly
    QFont font;
    font.setFamily("Hack Nerd Font Mono");
    font.setPointSize(10);
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    setTerminalFont(font);
    
    // Set terminal size hint
    setTerminalSizeHint(false);
    
    // Enable scroll on output
    setScrollBarPosition(QTermWidget::ScrollBarRight);
    
    // Set color scheme based on theme
    setTheme(currentTheme);
    
    // Setup copy/paste shortcuts using QShortcut (more reliable than keyPressEvent)
    copyShortcut = new QShortcut(QKeySequence("Ctrl+Shift+C"), this);
    connect(copyShortcut, &QShortcut::activated, this, &QTermWidget::copyClipboard);
    
    pasteShortcut = new QShortcut(QKeySequence("Ctrl+Shift+V"), this);
    connect(pasteShortcut, &QShortcut::activated, this, &QTermWidget::pasteClipboard);
    
    // Set shell if provided
    if (!shell.isEmpty()) {
        shellPath = shell;
        setShellProgram(shell);
        
        // Check if this is R and set appropriate arguments
        QString shellName = QFileInfo(shell).fileName().toLower();
        if (shellName == "r") {
            QStringList rArgs;
            rArgs << "--interactive" << "--no-save";
            QTermWidget::setArgs(rArgs);
        }
    } else {
        // Use system default shell
        QString defaultShell = qgetenv("SHELL");
        if (defaultShell.isEmpty()) {
            defaultShell = "/bin/bash";
        }
        shellPath = defaultShell;
        setShellProgram(defaultShell);
    }
    
    // Set environment variables for proper UTF-8 support
    QProcessEnvironment sysEnv = QProcessEnvironment::systemEnvironment();
    QStringList env;
    
    // Convert system environment to string list, skipping locale vars we'll override
    for (const QString &key : sysEnv.keys()) {
        if (key != "LANG" && key != "LC_ALL" && key != "TERM" && key != "R_PROFILE_USER") {
            env << QString("%1=%2").arg(key, sysEnv.value(key));
        }
    }
    
    // Add UTF-8 locale settings
    env << "LANG=en_US.UTF-8";
    env << "LC_ALL=en_US.UTF-8";
    env << "TERM=xterm-256color";
    
    // Setup R profile for silent loading
    QString shellName = QFileInfo(shellPath).fileName().toLower();
    if (shellName == "r") {
        QString initScriptPath = QDir::tempPath() + "/q_init_" + QString::number(QCoreApplication::applicationPid()) + ".R";
        QFile initScript(initScriptPath);
        if (initScript.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&initScript);
            out << "local({\n";
            out << "  orig_prof <- Sys.getenv('Q_ORIGINAL_R_PROFILE_USER')\n";
            out << "  if (nzchar(orig_prof) && file.exists(orig_prof)) {\n";
            out << "    source(orig_prof)\n";
            out << "  } else {\n";
            out << "    if (file.exists('.Rprofile')) source('.Rprofile')\n";
            out << "    else if (file.exists(file.path(Sys.getenv('HOME'), '.Rprofile'))) source(file.path(Sys.getenv('HOME'), '.Rprofile'))\n";
            out << "  }\n";
            out << "  if (requireNamespace('qide', quietly=TRUE)) {\n";
            out << "    library(qide)\n";
            out << "    qide::init_monitor('/tmp/q_env.json')\n";
            out << "  }\n";
            out << "})\n";
            initScript.close();
            
            QString originalProfile = sysEnv.value("R_PROFILE_USER");
            if (!originalProfile.isEmpty()) {
                env << "Q_ORIGINAL_R_PROFILE_USER=" + originalProfile;
            }
            env << "R_PROFILE_USER=" + initScriptPath;
        }
    }
    
    setEnvironment(env);
    
    // Set working directory to user's home
    setWorkingDirectory(QDir::homePath());
    
    // Set key bindings (default should work, but we can customize if needed)
    setKeyBindings("default");
    
    // Disable bracketed paste mode to avoid ^[[200~ sequences when pasting
    disableBracketedPasteMode(true);
    
    // Start the shell
    startShellProgram();
}

TerminalWidget::~TerminalWidget()
{
    // QTermWidget handles cleanup
}

void TerminalWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = new QMenu(this);
    
    // Copy action
    QAction *copyAction = menu->addAction(tr("Copy"));
    copyAction->setShortcut(QKeySequence("Ctrl+Shift+C"));
    copyAction->setEnabled(selectedText().length() > 0);
    connect(copyAction, &QAction::triggered, this, &QTermWidget::copyClipboard);
    
    // Paste action
    QAction *pasteAction = menu->addAction(tr("Paste"));
    pasteAction->setShortcut(QKeySequence("Ctrl+Shift+V"));
    connect(pasteAction, &QAction::triggered, this, &QTermWidget::pasteClipboard);
    
    menu->addSeparator();
    
    // Clear action
    QAction *clearAction = menu->addAction(tr("Clear"));
    connect(clearAction, &QAction::triggered, this, [this]() {
        QString shellName = QFileInfo(shellPath).fileName().toLower();
        if (shellName == "r") {
            executeCommand("clear()");
        } else {
            executeCommand("clear");
        }
    });
    
    menu->exec(event->globalPos());
    delete menu;
}

void TerminalWidget::setTheme(const EditorTheme &theme)
{
    currentTheme = theme;
    
    // Also set the stylesheet for this widget specifically to ensure background matches
    // This helps if QTermWidget has margins or transparency issues
    setStyleSheet(QString("TerminalWidget { background-color: %1; color: %2; }")
                  .arg(theme.background.name())
                  .arg(theme.foreground.name()));

    // Create a custom color scheme for QTermWidget based on the theme
    QString schemeName = theme.name;
    // Sanitize name for filename
    QString safeName = schemeName;
    safeName.replace(QRegularExpression("[^a-zA-Z0-9_]"), "_");
    
    // Prepare content
    QString content;
    QTextStream out(&content);
    
    auto writeColor = [&](const QString &section, const QColor &color) {
        out << "[" << section << "]\n";
        out << "Color=" << color.red() << "," << color.green() << "," << color.blue() << "\n";
        out << "Transparency=false\n\n";
    };
    
    out << "[General]\n";
    out << "Description=" << safeName << "\n";
    out << "Opacity=1\n\n";
    
    writeColor("Background", theme.background);
    writeColor("Foreground", theme.foreground);
    writeColor("BackgroundIntense", theme.background);
    writeColor("ForegroundIntense", theme.foreground);
    
    writeColor("Color0", theme.color_01);
    writeColor("Color1", theme.color_02);
    writeColor("Color2", theme.color_03);
    writeColor("Color3", theme.color_04);
    writeColor("Color4", theme.color_05);
    writeColor("Color5", theme.color_06);
    writeColor("Color6", theme.color_07);
    writeColor("Color7", theme.color_08);
    
    writeColor("Color0Intense", theme.color_09);
    writeColor("Color1Intense", theme.color_10);
    writeColor("Color2Intense", theme.color_11);
    writeColor("Color3Intense", theme.color_12);
    writeColor("Color4Intense", theme.color_13);
    writeColor("Color5Intense", theme.color_14);
    writeColor("Color6Intense", theme.color_15);
    writeColor("Color7Intense", theme.color_16);
    
    // Try to write to both qtermwidget5 and qtermwidget6 locations
    // as we don't know which version is linked at runtime
    QString dataLoc = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString configLoc = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString home = QDir::homePath();
    
    QStringList schemeDirs;
    // Standard XDG data locations
    schemeDirs << dataLoc + "/qtermwidget6/color-schemes";
    schemeDirs << dataLoc + "/qtermwidget5/color-schemes";
    // Standard XDG config locations
    schemeDirs << configLoc + "/qtermwidget6/color-schemes";
    schemeDirs << configLoc + "/qtermwidget5/color-schemes";
    // Explicit home locations (fallback)
    schemeDirs << home + "/.local/share/qtermwidget6/color-schemes";
    schemeDirs << home + "/.local/share/qtermwidget5/color-schemes";
    schemeDirs << home + "/.config/qtermwidget6/color-schemes";
    schemeDirs << home + "/.config/qtermwidget5/color-schemes";
    // Application directory (portable)
    schemeDirs << QCoreApplication::applicationDirPath() + "/color-schemes";
    
    bool saved = false;
    for (const QString &dirPath : schemeDirs) {
        QDir dir(dirPath);
        if (!dir.exists()) {
            if (!dir.mkpath(".")) continue;
        }
        
        QFile file(dir.filePath(safeName + ".colorscheme"));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream fileOut(&file);
            fileOut << content;
            file.close();
            saved = true;
            // Only log once to avoid spam
            static QStringList loggedSchemes;
            if (!loggedSchemes.contains(file.fileName())) {
                qDebug() << "Saved color scheme to:" << file.fileName();
                loggedSchemes << file.fileName();
            }
        }
    }
    
    if (saved) {
        // Check if the scheme is actually available to QTermWidget
        QStringList schemes = availableColorSchemes();
        if (schemes.contains(safeName)) {
            setColorScheme(safeName);
        } else {
            // Try setting it anyway - sometimes the list is cached but loading works
            // if the file exists in the right place
            setColorScheme(safeName);
            
            // Verify if it worked (optional, but good for debugging)
            // If it didn't work, we might want to fallback, but QTermWidget usually
            // keeps the previous scheme or uses a default if the new one fails.
            
            // If we really want to force a fallback if it failed:
            // But we can't easily check if setColorScheme succeeded as it returns void.
            
            qWarning() << "Attempted to set scheme" << safeName << "even though it wasn't listed.";
            qWarning() << "Available schemes:" << schemes;
        }
    } else {
        // Fallback
        int brightness = theme.background.red() + theme.background.green() + theme.background.blue();
        if (brightness < 384) {
            setColorScheme("Linux");
        } else {
            setColorScheme("WhiteOnBlack");
        }
    }
}

void TerminalWidget::setArgs(const QStringList &args)
{
    QTermWidget::setArgs(args);
}

void TerminalWidget::writeToShell(const QString &text)
{
    sendText(text);
}

void TerminalWidget::executeCommand(const QString &command)
{
    sendText(command + "\n");
}



