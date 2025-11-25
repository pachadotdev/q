#include "thememanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QCoreApplication>
#include <QApplication>
#include <QPalette>
#include <QDir>
#include <QFileInfo>

ThemeManager& ThemeManager::instance()
{
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager()
    : currentThemeName("Dracula")
{
    themesDir = findThemesDirectory();
    // Don't initialize built-in themes, only use Gogh JSON themes
    // initializeThemes();
    
    // Scan for available JSON themes
    scanJsonThemes();
    
    // Set default theme to Dracula if it exists, otherwise use first available
    if (jsonThemeNames.contains("Dracula")) {
        currentThemeName = "Dracula";
    } else if (!jsonThemeNames.isEmpty()) {
        currentThemeName = jsonThemeNames.first();
    }
    // Apply the selected theme to the application at startup
    EditorTheme initial = getTheme(currentThemeName);
    if (!initial.name.isEmpty()) {
        applyTheme(initial);
    }
}

void ThemeManager::initializeThemes()
{
    // Light theme (default)
    EditorTheme light;
    light.name = "Light";
    light.background = QColor(255, 255, 255);
    light.foreground = QColor(0, 0, 0);
    light.selection = QColor(173, 214, 255);
    light.lineHighlight = QColor(245, 245, 245);
    light.lineNumber = QColor(128, 128, 128);
    light.lineNumberBg = QColor(240, 240, 240);
    light.keyword = QColor(0, 0, 255);
    light.function = QColor(0, 102, 153);
    light.string = QColor(0, 153, 0);
    light.number = QColor(200, 0, 200);
    light.comment = QColor(0, 128, 0);
    light.operator_ = QColor(139, 0, 0);
    themes["Light"] = light;
    
    // Dracula (popular dark theme)
    EditorTheme dracula;
    dracula.name = "Dracula";
    dracula.background = QColor("#282a36");
    dracula.foreground = QColor("#f8f8f2");
    dracula.selection = QColor("#44475a");
    dracula.lineHighlight = QColor("#44475a");
    dracula.lineNumber = QColor("#6272a4");
    dracula.lineNumberBg = QColor("#21222c");
    dracula.keyword = QColor("#ff79c6");
    dracula.function = QColor("#50fa7b");
    dracula.string = QColor("#f1fa8c");
    dracula.number = QColor("#bd93f9");
    dracula.comment = QColor("#6272a4");
    dracula.operator_ = QColor("#ff79c6");
    themes["Dracula"] = dracula;
    
    // Monokai
    EditorTheme monokai;
    monokai.name = "Monokai";
    monokai.background = QColor("#272822");
    monokai.foreground = QColor("#f8f8f2");
    monokai.selection = QColor("#49483e");
    monokai.lineHighlight = QColor("#3e3d32");
    monokai.lineNumber = QColor("#90908a");
    monokai.lineNumberBg = QColor("#232321");
    monokai.keyword = QColor("#f92672");
    monokai.function = QColor("#a6e22e");
    monokai.string = QColor("#e6db74");
    monokai.number = QColor("#ae81ff");
    monokai.comment = QColor("#75715e");
    monokai.operator_ = QColor("#f92672");
    themes["Monokai"] = monokai;
    
    // Solarized Dark
    EditorTheme solarizedDark;
    solarizedDark.name = "Solarized Dark";
    solarizedDark.background = QColor("#002b36");
    solarizedDark.foreground = QColor("#839496");
    solarizedDark.selection = QColor("#073642");
    solarizedDark.lineHighlight = QColor("#073642");
    solarizedDark.lineNumber = QColor("#586e75");
    solarizedDark.lineNumberBg = QColor("#002b36");
    solarizedDark.keyword = QColor("#268bd2");
    solarizedDark.function = QColor("#2aa198");
    solarizedDark.string = QColor("#859900");
    solarizedDark.number = QColor("#d33682");
    solarizedDark.comment = QColor("#586e75");
    solarizedDark.operator_ = QColor("#cb4b16");
    themes["Solarized Dark"] = solarizedDark;
    
    // Nord
    EditorTheme nord;
    nord.name = "Nord";
    nord.background = QColor("#2e3440");
    nord.foreground = QColor("#d8dee9");
    nord.selection = QColor("#434c5e");
    nord.lineHighlight = QColor("#3b4252");
    nord.lineNumber = QColor("#4c566a");
    nord.lineNumberBg = QColor("#2e3440");
    nord.keyword = QColor("#81a1c1");
    nord.function = QColor("#88c0d0");
    nord.string = QColor("#a3be8c");
    nord.number = QColor("#b48ead");
    nord.comment = QColor("#616e88");
    nord.operator_ = QColor("#81a1c1");
    themes["Nord"] = nord;
    
    // Gruvbox Dark
    EditorTheme gruvbox;
    gruvbox.name = "Gruvbox Dark";
    gruvbox.background = QColor("#282828");
    gruvbox.foreground = QColor("#ebdbb2");
    gruvbox.selection = QColor("#504945");
    gruvbox.lineHighlight = QColor("#3c3836");
    gruvbox.lineNumber = QColor("#928374");
    gruvbox.lineNumberBg = QColor("#1d2021");
    gruvbox.keyword = QColor("#fb4934");
    gruvbox.function = QColor("#b8bb26");
    gruvbox.string = QColor("#b8bb26");
    gruvbox.number = QColor("#d3869b");
    gruvbox.comment = QColor("#928374");
    gruvbox.operator_ = QColor("#fe8019");
    themes["Gruvbox Dark"] = gruvbox;
}

QString ThemeManager::findThemesDirectory() const
{
    const QString path = QCoreApplication::applicationDirPath() + "/gogh-themes";
    QDir dir(path);
    if (dir.exists() && !dir.entryList(QStringList() << "*.json", QDir::Files).isEmpty()) {
        return dir.absolutePath();
    }

    qWarning() << "Themes directory not found at" << path;
    return QString();
}

void ThemeManager::scanJsonThemes()
{
    if (themesDir.isEmpty()) {
        qWarning() << "Themes directory not set, cannot scan for JSON themes";
        return;
    }
    
    QDir dir(themesDir);
    QStringList filters;
    filters << "*.json";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable);
    
    jsonThemeNames.clear();
    for (const QFileInfo &fileInfo : files) {
        // Read the JSON file to get the theme name
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QString themeName = obj["name"].toString();
                if (!themeName.isEmpty()) {
                    jsonThemeNames.append(themeName);
                }
            }
        }
    }
    
    jsonThemeNames.sort(Qt::CaseInsensitive);
    // qDebug() << "Found" << jsonThemeNames.size() << "JSON themes";
}

QStringList ThemeManager::availableThemes() const
{
    // Only return Gogh JSON themes, no built-in themes
    QStringList allThemes = jsonThemeNames;
    allThemes.sort(Qt::CaseInsensitive);
    return allThemes;
}

EditorTheme ThemeManager::getTheme(const QString &name) const
{
    // Check if it's already cached
    if (themes.contains(name)) {
        return themes.value(name);
    }
    
    // Check if it's a JSON theme that needs to be loaded
    if (jsonThemeNames.contains(name)) {
        // Load it on-demand
        EditorTheme theme = loadThemeFromJson(name);
        if (!theme.name.isEmpty()) {
            // Cache it for future use
            const_cast<ThemeManager*>(this)->themes[name] = theme;
            return theme;
        }
    }
    
    // Return a fallback theme
    qWarning() << "Theme not found:" << name << ", returning fallback theme";
    EditorTheme fallback;
    fallback.name = "Fallback";
    fallback.background = QColor("#282a36");
    fallback.foreground = QColor("#f8f8f2");
    fallback.selection = QColor("#44475a");
    fallback.lineHighlight = QColor("#44475a");
    fallback.lineNumber = QColor("#6272a4");
    fallback.lineNumberBg = QColor("#21222c");
    fallback.keyword = QColor("#ff79c6");
    fallback.function = QColor("#50fa7b");
    fallback.string = QColor("#f1fa8c");
    fallback.number = QColor("#bd93f9");
    fallback.comment = QColor("#6272a4");
    fallback.operator_ = QColor("#ff79c6");
    return fallback;
}

EditorTheme ThemeManager::currentTheme() const
{
    return getTheme(currentThemeName);
}

void ThemeManager::setCurrentTheme(const QString &name)
{
    EditorTheme theme = getTheme(name);
    if (!theme.name.isEmpty()) {
        currentThemeName = name;
        applyTheme(theme);
    }
}

QString ThemeManager::toStyleSheet(const EditorTheme &theme) const
{
    // Create a comprehensive stylesheet for the entire application
    QString stylesheet;
    
    // Main application colors
    stylesheet += QString(
        "QMainWindow, QWidget {"
        "   background-color: %1;"
        "   color: %2;"
        "}"
        "QPlainTextEdit, QTextEdit {"
        "   background-color: %1;"
        "   color: %2;"
        "   selection-background-color: %3;"
        "   font-family: 'Hack', 'Courier New', monospace;"
        "   font-size: 10pt;"
        "}"
        "QMenuBar {"
        "   background-color: %1;"
        "   color: %2;"
        "}"
        "QMenuBar::item:selected {"
        "   background-color: %3;"
        "}"
        "QMenu {"
        "   background-color: %1;"
        "   color: %2;"
        "   border: 1px solid %4;"
        "}"
        "QMenu::item:selected {"
        "   background-color: %3;"
        "}"
        "QDockWidget {"
        "   background-color: %1;"
        "   color: %2;"
        "}"
        "QDockWidget::title {"
        "   background-color: %5;"
        "   color: %2;"
        "   padding: 4px;"
        "}"
        "QTabWidget::pane {"
        "   border: 1px solid %4;"
        "   background-color: %1;"
        "}"
        "QTabBar::tab {"
        "   background-color: %5;"
        "   color: %2;"
        "   padding: 5px 10px;"
        "   border: 1px solid %4;"
        "}"
        "QTabBar::tab:selected {"
        "   background-color: %1;"
        "   border-bottom-color: %1;"
        "}"
        "QToolBar {"
        "   background-color: %5;"
        "   color: %2;"
        "   border: none;"
        "}"
        "QStatusBar {"
        "   background-color: %5;"
        "   color: %2;"
        "}"
        "QScrollBar:vertical {"
        "   background: %5;"
        "   width: 12px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: %4;"
        "   min-height: 20px;"
        "}"
        "QScrollBar:horizontal {"
        "   background: %5;"
        "   height: 12px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "   background: %4;"
        "   min-width: 20px;"
        "}"
        "QPushButton {"
        "   background-color: %5;"                              // use lineHighlight for button background
        "   color: %2;"
        "   border: 1px solid %4;"                              // subtle border using lineNumber
        "   padding: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: %3;"
        "}"
        "QToolButton {"
        "   background-color: transparent;"
        "   color: %2;"
        "   border: none;"
        "   padding: 4px;"
        "}"
        "QToolButton:hover {"
        "   background-color: %5;"
        "}"
        "QLineEdit, QComboBox {"
        "   background-color: %5;"
        "   color: %2;"
        "   border: 1px solid %4;"
        "   padding: 3px;"
        "}"
        "QListWidget {"
        "   background-color: %1;"
        "   color: %2;"
        "   border: 1px solid %4;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: %3;"
        "}"
        "QTreeView, QTableView {"
        "   background-color: %1;"
        "   color: %2;"
        "   alternate-background-color: %5;"
        "   border: 1px solid %4;"
        "}"
        "QHeaderView::section {"
        "   background-color: %5;"
        "   color: %2;"
        "   padding: 4px;"
        "   border: 1px solid %4;"
        "}"
    ).arg(theme.background.name())
     .arg(theme.foreground.name())
     .arg(theme.selection.name())
     .arg(theme.lineNumber.name())
     .arg(theme.lineHighlight.name());
    
    return stylesheet;
}

void ThemeManager::applyTheme(const EditorTheme &theme) const
{
    // Install stylesheet
    qApp->setStyleSheet(toStyleSheet(theme));

    // Build and apply a QPalette that matches the theme so native widgets
    // and style elements render with correct colors.
    QPalette pal;
    pal.setColor(QPalette::Window, theme.background);
    pal.setColor(QPalette::WindowText, theme.foreground);
    pal.setColor(QPalette::Base, theme.background);
    pal.setColor(QPalette::Text, theme.foreground);
    pal.setColor(QPalette::Button, theme.lineHighlight);
    pal.setColor(QPalette::ButtonText, theme.foreground);
    pal.setColor(QPalette::Highlight, theme.selection);
    pal.setColor(QPalette::HighlightedText, theme.foreground);
    pal.setColor(QPalette::ToolTipBase, theme.lineHighlight);
    pal.setColor(QPalette::ToolTipText, theme.foreground);

    qApp->setPalette(pal);
}

EditorTheme ThemeManager::loadThemeFromJson(const QString &themeName) const
{
    if (themesDir.isEmpty()) {
        qWarning() << "Themes directory not set";
        return themes.value("Light", themes.value("Dracula"));
    }
    
    // Find the JSON file with this theme name
    QDir dir(themesDir);
    QStringList filters;
    filters << "*.json";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files | QDir::Readable);
    
    QString targetFile;
    for (const QFileInfo &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj["name"].toString() == themeName) {
                    targetFile = fileInfo.absoluteFilePath();
                    break;
                }
            }
        }
    }
    
    if (targetFile.isEmpty()) {
        qWarning() << "Theme file not found for:" << themeName;
        return themes.value("Light", themes.value("Dracula"));
    }
    
    // Re-open and parse the file
    QFile file(targetFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open theme file:" << targetFile;
        return themes.value("Light", themes.value("Dracula"));
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON in theme file:" << targetFile;
        return themes.value("Light", themes.value("Dracula"));
    }
    
    EditorTheme theme = parseJsonTheme(doc.object());
    
    if (theme.name.isEmpty()) {
        qWarning() << "Failed to parse theme:" << themeName;
        return themes.value("Light", themes.value("Dracula"));
    }
    
    // qDebug() << "Loaded theme:" << theme.name << "from" << QFileInfo(targetFile).fileName();
    
    return theme;
}

EditorTheme ThemeManager::parseJsonTheme(const QJsonObject &obj) const
{
    EditorTheme theme;
    
    try {
        theme.name = obj["name"].toString();
        theme.author = obj["author"].toString();
        theme.variant = obj["variant"].toString();
        
        // Parse colors with defaults
        theme.background = QColor(obj["background"].toString("#000000"));
        theme.foreground = QColor(obj["foreground"].toString("#FFFFFF"));
        theme.cursor = QColor(obj["cursor"].toString(theme.foreground.name()));
        
        // Parse ANSI colors
        theme.color_01 = QColor(obj["color_01"].toString("#000000"));
        theme.color_02 = QColor(obj["color_02"].toString("#FF0000"));
        theme.color_03 = QColor(obj["color_03"].toString("#00FF00"));
        theme.color_04 = QColor(obj["color_04"].toString("#FFFF00"));
        theme.color_05 = QColor(obj["color_05"].toString("#0000FF"));
        theme.color_06 = QColor(obj["color_06"].toString("#FF00FF"));
        theme.color_07 = QColor(obj["color_07"].toString("#00FFFF"));
        theme.color_08 = QColor(obj["color_08"].toString("#FFFFFF"));
        theme.color_09 = QColor(obj["color_09"].toString("#808080"));
        theme.color_10 = QColor(obj["color_10"].toString("#FF8080"));
        theme.color_11 = QColor(obj["color_11"].toString("#80FF80"));
        theme.color_12 = QColor(obj["color_12"].toString("#FFFF80"));
        theme.color_13 = QColor(obj["color_13"].toString("#8080FF"));
        theme.color_14 = QColor(obj["color_14"].toString("#FF80FF"));
        theme.color_15 = QColor(obj["color_15"].toString("#80FFFF"));
        theme.color_16 = QColor(obj["color_16"].toString("#FFFFFF"));
        
        // Validate colors
        if (!theme.background.isValid()) {
            qWarning() << "Invalid background color, using default";
            theme.background = QColor("#000000");
        }
        if (!theme.foreground.isValid()) {
            qWarning() << "Invalid foreground color, using default";
            theme.foreground = QColor("#FFFFFF");
        }
        
        // Set selection color (slightly lighter/darker than background)
        QColor bg = theme.background;
        int selectionLightness = bg.lightness() + (bg.lightness() < 128 ? 20 : -20);
        selectionLightness = qBound(0, selectionLightness, 255);
        
        // For grayscale colors (hue = -1), use RGB manipulation instead of HSL
        if (bg.hue() == -1) {
            int r = qBound(0, bg.red() + (bg.lightness() < 128 ? 20 : -20), 255);
            int g = qBound(0, bg.green() + (bg.lightness() < 128 ? 20 : -20), 255);
            int b = qBound(0, bg.blue() + (bg.lightness() < 128 ? 20 : -20), 255);
            theme.selection = QColor(r, g, b);
        } else {
            theme.selection = QColor::fromHsl(bg.hue(), bg.saturation(), selectionLightness);
        }
        
        // Validate selection color
        if (!theme.selection.isValid()) {
            qWarning() << "Invalid selection color calculated, using fallback";
            theme.selection = QColor(bg.red() + 20, bg.green() + 20, bg.blue() + 20);
        }
        
        // Set line highlight (slightly different from background)
        int lineHighlightLightness = bg.lightness() + (bg.lightness() < 128 ? 10 : -10);
        lineHighlightLightness = qBound(0, lineHighlightLightness, 255);
        
        if (bg.hue() == -1) {
            int r = qBound(0, bg.red() + (bg.lightness() < 128 ? 10 : -10), 255);
            int g = qBound(0, bg.green() + (bg.lightness() < 128 ? 10 : -10), 255);
            int b = qBound(0, bg.blue() + (bg.lightness() < 128 ? 10 : -10), 255);
            theme.lineHighlight = QColor(r, g, b);
        } else {
            theme.lineHighlight = QColor::fromHsl(bg.hue(), bg.saturation(), lineHighlightLightness);
        }
        
        // Validate line highlight color
        if (!theme.lineHighlight.isValid()) {
            qWarning() << "Invalid line highlight color calculated, using fallback";
            theme.lineHighlight = QColor(bg.red() + 10, bg.green() + 10, bg.blue() + 10);
        }
        
        // Set line number colors
        theme.lineNumber = theme.color_09; // Bright black
        theme.lineNumberBg = theme.background;
        
        // Map ANSI colors to syntax highlighting
        theme.keyword = theme.color_06;   // Magenta - keywords
        theme.function = theme.color_03;  // Green - functions
        theme.string = theme.color_04;    // Yellow - strings
        theme.number = theme.color_05;    // Blue - numbers
        theme.comment = theme.color_09;   // Bright black - comments
        theme.operator_ = theme.color_02; // Red - operators
        
    } catch (const std::exception &e) {
        qWarning() << "Exception parsing theme:" << e.what();
        theme.name.clear(); // Mark as invalid
    } catch (...) {
        qWarning() << "Unknown exception parsing theme";
        theme.name.clear(); // Mark as invalid
    }
    
    return theme;
}
