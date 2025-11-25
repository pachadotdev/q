#ifndef ENVIRONMENTPANE_H
#define ENVIRONMENTPANE_H

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>

#include <QFileSystemWatcher>

class TerminalWidget;

class EnvironmentPane : public QWidget
{
    Q_OBJECT

public:
    explicit EnvironmentPane(TerminalWidget *terminal, QWidget *parent = nullptr);
    ~EnvironmentPane();

public slots:
    void refreshEnvironment();
    void deleteCheckedItems();

private slots:
    void onEnvironmentFileChanged(const QString &path);
    void clearAllItems();
    void runGC();

private:
    TerminalWidget *terminal;
    QTreeWidget *treeWidget;
    QPushButton *refreshButton;
    QPushButton *deleteButton;
    QPushButton *clearButton;
    QPushButton *gcButton;
    QLabel *memoryLabel;
    
    QString envFilePath;
    QFileSystemWatcher *fileWatcher;

    void parseEnvironmentData(const QByteArray &jsonData);
    void setupEnvironmentMonitor();
    QString formatSize(double bytes);
};

#endif // ENVIRONMENTPANE_H
