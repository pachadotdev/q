#include "rsyntaxhighlighter.h"

RSyntaxHighlighter::RSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Initialize with current theme
    setTheme(ThemeManager::instance().currentTheme());
}

void RSyntaxHighlighter::setTheme(const EditorTheme &theme)
{
    highlightingRules.clear();
    
    HighlightingRule rule;
    
    // Keywords
    keywordFormat.setForeground(theme.keyword);
    keywordFormat.setFontWeight(QFont::Bold);
    
    QStringList keywordPatterns = {
        "\\bif\\b", "\\belse\\b", "\\bfor\\b", "\\bwhile\\b", "\\brepeat\\b",
        "\\bfunction\\b", "\\breturn\\b", "\\bnext\\b", "\\bbreak\\b",
        "\\bTRUE\\b", "\\bFALSE\\b", "\\bNULL\\b", "\\bNA\\b", "\\bNaN\\b",
        "\\bInf\\b", "\\bin\\b"
    };
    
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }
    
    // Functions (word followed by opening parenthesis)
    functionFormat.setForeground(theme.function);
    functionFormat.setFontItalic(false);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_\\.]+(?=\\s*\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);
    
    // Numbers
    numberFormat.setForeground(theme.number);
    rule.pattern = QRegularExpression("\\b[0-9]+\\.?[0-9]*([eE][-+]?[0-9]+)?\\b");
    rule.format = numberFormat;
    highlightingRules.append(rule);
    
    // Operators
    operatorFormat.setForeground(theme.operator_);
    QStringList operatorPatterns = {
        "\\+", "-", "\\*", "/", "\\^", "%%", "%/%",
        "==", "!=", "<", ">", "<=", ">=",
        "\\&", "\\|", "!", "&&", "\\|\\|",
        "<-", "<<-", "->", "->>", "=", "~"
    };
    
    for (const QString &pattern : operatorPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = operatorFormat;
        highlightingRules.append(rule);
    }
    
    // Strings (double quotes)
    stringFormat.setForeground(theme.string);
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    rule.format = stringFormat;
    highlightingRules.append(rule);
    
    // Strings (single quotes)
    rule.pattern = QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
    rule.format = stringFormat;
    highlightingRules.append(rule);
    
    // Comments
    commentFormat.setForeground(theme.comment);
    commentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = commentFormat;
    highlightingRules.append(rule);
}

void RSyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
