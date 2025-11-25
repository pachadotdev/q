#include "terminalwidget.h"
#include <QDir>
#include <QFileInfo>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QProcessEnvironment>

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
    connect(clearAction, &QAction::triggered, this, &QTermWidget::clear);
    
    menu->exec(event->globalPos());
    delete menu;
}

void TerminalWidget::setTheme(const EditorTheme &theme)
{
    currentTheme = theme;
    
    // Set a built-in color scheme that's close to our theme
    // We'll use a simple dark/light detection
    int brightness = theme.background.red() + theme.background.green() + theme.background.blue();
    
    if (brightness < 384) {
        // Dark theme - use a dark color scheme
        setColorScheme("Linux");
    } else {
        // Light theme  
        setColorScheme("WhiteOnBlack");
    }
    
    // Note: For full theme integration, you could create custom color scheme files
    // based on the theme colors and save them to ~/.local/share/qtermwidget6/color-schemes/
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



