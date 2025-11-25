#ifndef RSYNTAXHIGHLIGHTER_H
#define RSYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>
#include "thememanager.h"

class RSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit RSyntaxHighlighter(QTextDocument *parent = nullptr);
    void setTheme(const EditorTheme &theme);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;
    
    QTextCharFormat keywordFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat operatorFormat;
};

#endif // RSYNTAXHIGHLIGHTER_H
