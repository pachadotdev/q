#include "rprocess.h"
#include <QStandardPaths>
#include <QFileInfo>
#include <QDebug>

RProcess::RProcess(QObject *parent)
    : QObject(parent)
    , process(nullptr)
    , processingCommand(false)
    , waitingForFirstPrompt(true)
{
    findRExecutable();
}

RProcess::~RProcess()
{
    stop();
}

void RProcess::findRExecutable()
{
    // Try to find R executable
    QStringList possiblePaths;
    
#ifdef Q_OS_WIN
    possiblePaths << "C:/Program Files/R/R-4.3.2/bin/x64/R.exe"
                  << "C:/Program Files/R/R-4.3.1/bin/x64/R.exe"
                  << "C:/Program Files/R/R-4.2.3/bin/x64/R.exe";
#else
    possiblePaths << "/usr/bin/R"
                  << "/usr/local/bin/R"
                  << "/opt/R/bin/R";
#endif
    
    // Check system PATH
    QString rInPath = QStandardPaths::findExecutable("R");
    if (!rInPath.isEmpty()) {
        rExecutable = rInPath;
        return;
    }
    
    // Check known locations
    for (const QString &path : possiblePaths) {
        if (QFileInfo::exists(path)) {
            rExecutable = path;
            return;
        }
    }
    
    qWarning() << "R executable not found!";
}

void RProcess::start()
{
    if (rExecutable.isEmpty()) {
        emit errorReceived("R executable not found. Please install R.");
        return;
    }
    
    if (process) {
        stop();
    }
    
    process = new QProcess(this);
    
    waitingForFirstPrompt = true;
    processingCommand = false;
    commandQueue.clear();

    // Connect signals
    connect(process, &QProcess::readyReadStandardOutput,
            this, &RProcess::onReadyReadStandardOutput);
    connect(process, &QProcess::readyReadStandardError,
            this, &RProcess::onReadyReadStandardError);
    connect(process, &QProcess::started,
            this, &RProcess::onProcessStarted);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &RProcess::onProcessFinished);
    connect(process, &QProcess::errorOccurred,
            this, &RProcess::onProcessError);
    
    // Start R in interactive mode with specific options
    QStringList args;
    args << "--interactive"
         << "--no-save"
         << "--no-restore";
    
    process->start(rExecutable, args);
}

void RProcess::stop()
{
    if (process && process->state() != QProcess::NotRunning) {
        // Try graceful quit first
        executeCommand("q(save='no')");
        
        // Wait for process to finish
        if (!process->waitForFinished(3000)) {
            process->kill();
        }
    }
    
    if (process) {
        process->deleteLater();
        process = nullptr;
    }
}

void RProcess::executeCommand(const QString &command, bool isSilent)
{
    if (!process || process->state() != QProcess::Running) {
        emit errorReceived("R process is not running.");
        return;
    }
    
    // Add command to queue
    commandQueue.enqueue({command, isSilent});
    
    if (!processingCommand && !waitingForFirstPrompt) {
        processNextCommand();
    }
}

void RProcess::processNextCommand()
{
    if (commandQueue.isEmpty()) {
        processingCommand = false;
        return;
    }
    
    processingCommand = true;
    currentCommand = commandQueue.dequeue();
    
    // Write command to R process
    process->write(currentCommand.command.toUtf8());
    process->write("\n");
    process->waitForBytesWritten();
}

bool RProcess::isRunning() const
{
    return process && process->state() == QProcess::Running;
}

void RProcess::onReadyReadStandardOutput()
{
    if (!process) return;
    
    QString output = QString::fromUtf8(process->readAllStandardOutput());
    if (output.isEmpty()) return;
    
    if (waitingForFirstPrompt) {
        emit outputReceived(output);
        if (output.contains("> ") || output.endsWith(">")) {
            waitingForFirstPrompt = false;
            if (!commandQueue.isEmpty()) {
                processNextCommand();
            }
        }
        return;
    }

    // If current command is silent, we need to filter the output
    if (processingCommand && currentCommand.isSilent) {
        // We suppress output until we see the prompt
        // Note: This is a simple heuristic and might fail if the command output contains "> "
        // But for internal commands it should be fine.
        
        if (output.contains("> ") || output.endsWith(">")) {
            // Command finished
            emit commandFinished(currentCommand.command);
            
            // If there is output AFTER the prompt, we should emit it?
            // Or if there was output BEFORE the prompt that wasn't part of the silent command?
            // It's hard to separate. For now, we assume silent commands are truly silent
            // and we just swallow everything until the prompt.
            
            // If the queue is not empty, process next
            if (!commandQueue.isEmpty()) {
                processNextCommand();
            } else {
                processingCommand = false;
            }
        }
        // Else: swallow output
    } else {
        emit outputReceived(output);
        
        // Check for prompt to know if command finished
        // This is needed to keep sync with the queue
        // Note: R prompt can be customized, but we assume standard "> " or ">"
        // Also handle newlines before prompt
        QString trimmed = output.trimmed();
        if (output.contains("\n> ") || output.endsWith("> ") || output.endsWith(">")) {
             if (processingCommand) {
                 emit commandFinished(currentCommand.command);
                 
                 if (!commandQueue.isEmpty()) {
                     processNextCommand();
                 } else {
                     processingCommand = false;
                 }
             }
        }
    }
}

void RProcess::onReadyReadStandardError()
{
    if (!process) return;
    
    QString error = QString::fromUtf8(process->readAllStandardError());
    if (error.isEmpty()) return;

    if (processingCommand && currentCommand.isSilent) {
        // In silent mode, we suppress stderr as well to prevent it from appearing in the console
        // We can log it to debug for development purposes
        // qDebug() << "Silent command stderr:" << error;
    } else {
        emit errorReceived(error);
    }
}

void RProcess::onProcessStarted()
{
    emit started();
}

void RProcess::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString message;
    if (exitStatus == QProcess::CrashExit) {
        message = QString("R process crashed with exit code: %1").arg(exitCode);
    } else {
        message = QString("R process finished with exit code: %1").arg(exitCode);
    }
    
    emit outputReceived(message);
    emit finished();
}

void RProcess::onProcessError(QProcess::ProcessError error)
{
    QString errorMessage;
    
    switch (error) {
        case QProcess::FailedToStart:
            errorMessage = "Failed to start R process. Check if R is installed.";
            break;
        case QProcess::Crashed:
            errorMessage = "R process crashed.";
            break;
        case QProcess::Timedout:
            errorMessage = "R process timed out.";
            break;
        case QProcess::WriteError:
            errorMessage = "Write error to R process.";
            break;
        case QProcess::ReadError:
            errorMessage = "Read error from R process.";
            break;
        default:
            errorMessage = "Unknown R process error.";
    }
    
    emit errorReceived(errorMessage);
}
