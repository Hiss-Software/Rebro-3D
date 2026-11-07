#pragma once

#include <QDialog>
#include <QColor>

class QComboBox;
class QPushButton;
class QLabel;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(int theme, int bgMode, const QColor& custom,
                            QWidget* parent = nullptr);

signals:
    void themeChanged(int index);
    void backgroundChanged(int mode, const QColor& color);
    void languageChanged(const QString& code);

private slots:
    void onThemeCombo(int index);
    void onBgCombo(int index);
    void onPickColor();
    void onLangCombo(int index);

private:
    QComboBox* m_lang = nullptr;
    QComboBox* m_theme = nullptr;
    QComboBox* m_bg = nullptr;
    QPushButton* m_colorBtn = nullptr;
    QLabel* m_langNote = nullptr;
    QColor m_customColor;
};
