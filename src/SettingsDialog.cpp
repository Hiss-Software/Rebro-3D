#include "SettingsDialog.h"
#include "Localization.h"

#include <QVBoxLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QColorDialog>

using namespace Localization;

SettingsDialog::SettingsDialog(int theme, int bgMode, const QColor& custom,
                               QWidget* parent)
    : QDialog(parent)
    , m_customColor(custom.isValid() ? custom : QColor(230, 230, 225))
{
    setWindowTitle(Tr("Settings"));
    setModal(true);

    auto* layout = new QVBoxLayout(this);

    // --- Language ---
    auto* langBox = new QGroupBox(Tr("Language"), this);
    auto* langLay = new QVBoxLayout(langBox);
    m_lang = new QComboBox(langBox);
    // Language names are always written in their own language — do not translate them
    m_lang->addItem("Русский");
    m_lang->addItem("English");
    m_lang->addItem("中文");
    m_lang->addItem("العربية");
    m_lang->addItem("Español");
    m_lang->addItem("Deutsch");
    m_lang->addItem("Français");
    m_lang->addItem("日本語");
    m_lang->addItem("한국어");
    m_lang->addItem("Português");
    m_lang->setProperty("langCode", QStringList{
        "ru", "en", "zh", "ar", "es", "de", "fr", "ja", "ko", "pt"});
    const QStringList codes = m_lang->property("langCode").toStringList();
    const int langIdx = codes.indexOf(code());
    m_lang->setCurrentIndex(langIdx < 0 ? 0 : langIdx);
    langLay->addWidget(m_lang);
    layout->addWidget(langBox);

    // --- Interface Theme ---
    auto* themeBox = new QGroupBox(Tr("Interface Theme"), this);
    auto* themeLay = new QVBoxLayout(themeBox);
    m_theme = new QComboBox(themeBox);
    m_theme->addItem(Tr("Light"));
    m_theme->addItem(Tr("Dark"));
    m_theme->setCurrentIndex(theme < 0 ? 0 : theme);
    themeLay->addWidget(m_theme);
    layout->addWidget(themeBox);

    // --- Scene Background ---
    auto* bgBox = new QGroupBox(Tr("Scene Background"), this);
    auto* bgLay = new QVBoxLayout(bgBox);
    auto* bgRow = new QWidget(bgBox);
    auto* rowLay = new QHBoxLayout(bgRow);
    rowLay->setContentsMargins(0, 0, 0, 0);
    m_bg = new QComboBox(bgRow);
    m_bg->addItem(Tr("Light"));
    m_bg->addItem(Tr("Dark"));
    m_bg->addItem(Tr("Custom color..."));
    m_bg->setCurrentIndex(bgMode < 0 ? 0 : bgMode);
    rowLay->addWidget(m_bg);
    m_colorBtn = new QPushButton(bgRow);
    m_colorBtn->setFixedSize(48, 24);
    m_colorBtn->setStyleSheet(
        QString("background: %1; border: 1px solid gray; border-radius: 3px;")
            .arg(m_customColor.name(QColor::HexRgb)));
    rowLay->addWidget(m_colorBtn);
    bgLay->addWidget(bgRow);
    layout->addWidget(bgBox);

    connect(m_lang, &QComboBox::currentIndexChanged,
            this, &SettingsDialog::onLangCombo);
    connect(m_theme, &QComboBox::currentIndexChanged,
            this, &SettingsDialog::onThemeCombo);
    connect(m_bg, &QComboBox::currentIndexChanged,
            this, &SettingsDialog::onBgCombo);
    connect(m_colorBtn, &QPushButton::clicked,
            this, &SettingsDialog::onPickColor);
}

void SettingsDialog::onLangCombo(int index) {
    const QStringList codes = m_lang->property("langCode").toStringList();
    if (index < 0 || index >= codes.size()) return;
    if (codes[index] == code()) return;
    emit languageChanged(codes[index]);
    // Dialog is already open with old language — update it live
    setWindowTitle(Tr("Settings"));
    parentWidget()->setWindowTitle(parentWidget()->windowTitle());
}

void SettingsDialog::onThemeCombo(int index) {
    if (index >= 0)
        emit themeChanged(index);
}

void SettingsDialog::onBgCombo(int index) {
    if (index < 0) return;
    if (index == 2) {
        onPickColor(); // "custom color" selected — open color dialog immediately
        return;
    }
    emit backgroundChanged(index, QColor());
}

void SettingsDialog::onPickColor() {
    QColor picked = QColorDialog::getColor(m_customColor, this, Tr("Choose Color"));
    if (!picked.isValid()) return;
    m_customColor = picked;
    m_colorBtn->setStyleSheet(
        QString("background: %1; border: 1px solid gray; border-radius: 3px;")
            .arg(picked.name(QColor::HexRgb)));
    emit backgroundChanged(2, picked);
}