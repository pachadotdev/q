#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QPushButton>

class CodeEditor;
class FileBrowser;
class TerminalWidget;
class EnvironmentPane;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newFile();
    void openFile();
    void openDirectory();
    void createProject();
    void saveFile();
    void saveFileAs();
    void runCurrentLine();
    void runSelection();
    void runAll();
    void sourceFile();
    void changeTheme();
    void about();

private:
    void createMenus();
    void createToolBars();
    void createDockWidgets();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    void closeEvent(QCloseEvent *event) override;

    // Central widget
    QTabWidget *editorTabs;
    
    // Dock widgets
    QDockWidget *consoleDock;
    QDockWidget *filesDock;
    QDockWidget *plotsDock;
    QDockWidget *envDock;
    
    // Console tabs
    QTabWidget *consoleTabs;
    
    // Components
    TerminalWidget *console;
    FileBrowser *fileBrowser;
    EnvironmentPane *envPane;
    
    // Menus
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *codeMenu;
    QMenu *viewMenu;
    QMenu *helpMenu;
    
    // Toolbars
    QToolBar *mainToolBar;
    
    // Current file tracking
    QString currentFile;
    
    CodeEditor* getCurrentEditor();
    void addNewEditorTab(const QString &title = "Untitled");
};

#endif // MAINWINDOW_H
