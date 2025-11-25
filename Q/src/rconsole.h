#ifndef RCONSOLE_H
#define RCONSOLE_H

#include <QTextEdit>
#include <QString>
#include "thememanager.h"

class RProcess;

class RConsole : public QTextEdit
{
    Q_OBJECT

public:
    explicit RConsole(RProcess *rProcess, QWidget *parent = nullptr);
    void setTheme(const EditorTheme &theme);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onOutputReceived(const QString &output);
    void onErrorReceived(const QString &error);

private:
    RProcess *rProcess;
    QString currentInput;
    int promptPosition;
    EditorTheme currentTheme;
    bool isHidingOutput;
    QString lastCommand;
    QString visibleBuffer;
    
    void appendPrompt();
    void executeCurrentCommand();
    QString getCurrentCommand() const;
};

#endif // RCONSOLE_H
