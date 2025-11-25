#ifndef RPROCESS_H
#define RPROCESS_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QQueue>

class RProcess : public QObject
{
    Q_OBJECT

public:
    explicit RProcess(QObject *parent = nullptr);
    ~RProcess();

    void start();
    void stop();
    void executeCommand(const QString &command, bool isSilent = false);
    bool isRunning() const;

signals:
    void outputReceived(const QString &output);
    void errorReceived(const QString &error);
    void started();
    void finished();
    void commandFinished(const QString &command);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    struct RCommand {
        QString command;
        bool isSilent;
    };

    QProcess *process;
    QString rExecutable;
    QQueue<RCommand> commandQueue;
    RCommand currentCommand;
    bool processingCommand;
    bool waitingForFirstPrompt;
    
    void findRExecutable();
    void processNextCommand();
};

#endif // RPROCESS_H
