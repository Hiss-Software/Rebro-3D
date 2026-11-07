#pragma once

#include <QString>
#include <QtGlobal>

namespace Localization {

enum class Lang { Ru, En, Zh, Ar, Es, De, Fr, Ja, Ko, Pt };

inline Lang& current() {
    static Lang l = Lang::Ru;
    return l;
}

inline void setLanguage(const QString& code) {
    if (code == "en")      current() = Lang::En;
    else if (code == "zh") current() = Lang::Zh;
    else if (code == "ar") current() = Lang::Ar;
    else if (code == "es") current() = Lang::Es;
    else if (code == "de") current() = Lang::De;
    else if (code == "fr") current() = Lang::Fr;
    else if (code == "ja") current() = Lang::Ja;
    else if (code == "ko") current() = Lang::Ko;
    else if (code == "pt") current() = Lang::Pt;
    else                   current() = Lang::Ru;
}

inline QString code() {
    switch (current()) {
        case Lang::En: return "en";
        case Lang::Zh: return "zh";
        case Lang::Ar: return "ar";
        case Lang::Es: return "es";
        case Lang::De: return "de";
        case Lang::Fr: return "fr";
        case Lang::Ja: return "ja";
        case Lang::Ko: return "ko";
        case Lang::Pt: return "pt";
        default:       return "ru";
    }
}

inline bool isRtl() { return current() == Lang::Ar; }

struct Entry {
    const char* ru; const char* en; const char* zh; const char* ar;
    const char* es; const char* de; const char* fr;
    const char* ja; const char* ko; const char* pt;
};

inline const char* pick(const Entry& e) {
    switch (current()) {
        case Lang::En: return e.en;
        case Lang::Zh: return e.zh;
        case Lang::Ar: return e.ar;
        case Lang::Es: return e.es;
        case Lang::De: return e.de;
        case Lang::Fr: return e.fr;
        case Lang::Ja: return e.ja;
        case Lang::Ko: return e.ko;
        case Lang::Pt: return e.pt;
        default:       return e.ru;
    }
}

struct TrEntry { const char* key; Entry v; };

// Ключ — исходная русская строка. Нет записи — вернётся сам ключ.
inline const TrEntry kTable[] = {
    {"Файл",       {"Файл", "File", "文件", "ملف",
                    "Archivo", "Datei", "Fichier",
                    "ファイル", "파일", "Arquivo"}},
    {"Правка",     {"Правка", "Edit", "编辑", "تحرير",
                    "Editar", "Bearbeiten", "Édition",
                    "編集", "편집", "Editar"}},
    {"Добавить",   {"Добавить", "Add", "添加", "إضافة",
                    "Añadir", "Hinzufügen", "Ajouter",
                    "追加", "추가", "Adicionar"}},
    {"Справка",    {"Справка", "Help", "帮助", "مساعدة",
                    "Ayuda", "Hilfe", "Aide",
                    "ヘルプ", "도움말", "Ajuda"}},

    {"Новая сцена",     {"Новая сцена", "New scene", "新建场景", "مشهد جديد",
                        "Nueva escena", "Neue Szene", "Nouvelle scène",
                        "新規シーン", "새 장면", "Nova cena"}},
    {"Открыть...",      {"Открыть...", "Open...", "打开...", "فتح...",
                        "Abrir...", "Öffnen...", "Ouvrir...",
                        "開く...", "열기...", "Abrir..."}},
    {"Сохранить",       {"Сохранить", "Save", "保存", "حفظ",
                        "Guardar", "Speichern", "Enregistrer",
                        "保存", "저장", "Salvar"}},
    {"Сохранить как...",{"Сохранить как...", "Save as...", "另存为...", "حفظ باسم...",
                        "Guardar como...", "Speichern unter...", "Enregistrer sous...",
                        "名前を付けて保存...", "다른 이름으로 저장...", "Salvar como..."}},
    {"Экспорт...",      {"Экспорт...", "Export...", "导出...", "تصدير...",
                        "Exportar...", "Exportieren...", "Exporter...",
                        "エクスポート...", "내보내기...", "Exportar..."}},
    {"Настройки",       {"Настройки", "Settings", "设置", "الإعدادات",
                        "Ajustes", "Einstellungen", "Réglages",
                        "設定", "설정", "Configurações"}},
    {"Настройки...",    {"Настройки...", "Settings...", "设置...", "الإعدادات...",
                        "Ajustes...", "Einstellungen...", "Réglages...",
                        "設定...", "설정...", "Configurações..."}},
    {"Выход",           {"Выход", "Exit", "退出", "خروج",
                        "Salir", "Beenden", "Quitter",
                        "終了", "종료", "Sair"}},

    {"Отменить",        {"Отменить", "Undo", "撤销", "تراجع",
                        "Deshacer", "Rückgängig", "Annuler",
                        "元に戻す", "실행 취소", "Desfazer"}},
    {"Повторить",       {"Повторить", "Redo", "重做", "إعادة",
                        "Rehacer", "Wiederholen", "Rétablir",
                        "やり直す", "다시 실행", "Refazer"}},
    {"Удалить объект",  {"Удалить объект", "Delete object", "删除对象", "حذف الكائن",
                        "Eliminar objeto", "Objekt löschen", "Supprimer l'objet",
                        "オブジェクトを削除", "개체 삭제", "Excluir objeto"}},

    {"Куб",             {"Куб", "Cube", "立方体", "مكعب",
                        "Cubo", "Würfel", "Cube",
                        "立方体", "큐브", "Cubo"}},
    {"Плоскость",       {"Плоскость", "Plane", "平面", "مستوى",
                        "Plano", "Ebene", "Plan",
                        "平面", "평면", "Plano"}},
    {"Сфера",           {"Сфера", "Sphere", "球体", "كرة",
                        "Esfera", "Kugel", "Sphère",
                        "球体", "구체", "Esfera"}},
    {"Цилиндр",         {"Цилиндр", "Cylinder", "圆柱", "أسطوانة",
                        "Cilindro", "Zylinder", "Cylindre",
                        "円柱", "원기둥", "Cilindro"}},
    {"Конус",           {"Конус", "Cone", "圆锥", "مخروط",
                        "Cono", "Kegel", "Cône",
                        "円錐", "원뿔", "Cone"}},
    {"Тор",             {"Тор", "Torus", "圆环", "حلقة",
                        "Toro", "Torus", "Tore",
                        "トーラス", "토러스", "Toro"}},
    {"Модель из файла...", {"Модель из файла...", "Import model from file...",
                            "从文件导入模型...", "استيراد نموذج من ملف...",
                            "Importar modelo desde archivo...",
                            "Modell aus Datei importieren...",
                            "Importer un modèle depuis un fichier...",
                            "ファイルからモデルを読み込む...",
                            "파일에서 모델 가져오기...",
                            "Importar modelo de arquivo..."}},

    {"Справка...",      {"Справка...", "Help...", "帮助...", "مساعدة...",
                        "Ayuda...", "Hilfe...", "Aide...",
                        "ヘルプ...", "도움말...", "Ajuda..."}},
    {"О Rebro",         {"О Rebro", "About Rebro", "关于 Rebro", "حول Rebro",
                        "Acerca de Rebro", "Über Rebro", "À propos de Rebro",
                        "Rebro について", "Rebro 정보", "Sobre o Rebro"}},

    {"Главная",         {"Главная", "Main toolbar", "主工具栏", "الشريط الرئيسي",
                        "Barra principal", "Hauptleiste", "Barre principale",
                        "メインツールバー", "기본 도구 모음", "Barra principal"}},
    {"Добавить куб",    {"Добавить куб", "Add cube", "添加立方体", "إضافة مكعب",
                        "Añadir cubo", "Würfel hinzufügen", "Ajouter un cube",
                        "立方体を追加", "큐브 추가", "Adicionar cubo"}},
    {"Импорт модели",   {"Импорт модели", "Import model", "导入模型", "استيراد نموذج",
                        "Importar modelo", "Modell importieren", "Importer un modèle",
                        "モデルをインポート", "모델 가져오기", "Importar modelo"}},
    {"Экспорт",         {"Экспорт", "Export", "导出", "تصدير",
                        "Exportar", "Exportieren", "Exporter",
                        "エクスポート", "내보내기", "Exportar"}},

    {"Перемещение (G)", {"Перемещение (G)", "Move (G)", "移动 (G)", "تحريك (G)",
                        "Mover (G)", "Verschieben (G)", "Déplacer (G)",
                        "移動 (G)", "이동 (G)", "Mover (G)"}},
    {"Вращение (R)",    {"Вращение (R)", "Rotate (R)", "旋转 (R)", "تدوير (R)",
                        "Rotar (R)", "Drehen (R)", "Pivoter (R)",
                        "回転 (R)", "회전 (R)", "Rotacionar (R)"}},
    {"Масштаб (Shift+S)", {"Масштаб (Shift+S)", "Scale (Shift+S)",
                          "缩放 (Shift+S)", "تحجيم (Shift+S)",
                          "Escalar (Shift+S)", "Skalieren (Shift+S)",
                          "Échelle (Shift+S)", "スケール (Shift+S)",
                          "크기 조절 (Shift+S)", "Escala (Shift+S)"}},
    {"Вершины (V)",     {"Вершины (V)", "Vertices (V)", "顶点 (V)", "رؤوس (V)",
                        "Vértices (V)", "Vertices (V)", "Sommets (V)",
                        "頂点 (V)", "정점 (V)", "Vértices (V)"}},

    {"Быстрая камера",  {"Быстрая камера", "Fast camera", "快速相机", "كاميرا سريعة",
                        "Cámara rápida", "Schnelle Kamera", "Caméra rapide",
                        "高速カメラ", "빠른 카메라", "Câmera rápida"}},
    {"юн/с",            {"юн/с", "u/s", "单位/秒", "وحدة/ث",
                        "ud/s", "E/s", "u/s",
                        "単位/秒", "단위/초", "u/s"}},
    {"Скорость камеры WASD (юнитов в секунду)",
        {"Скорость камеры WASD (юнитов в секунду)",
         "WASD camera speed (units per second)",
         "WASD 相机速度（单位/秒）",
         "سرعة الكاميرا WASD (وحدة في الثانية)",
         "Velocidad de cámara WASD (unidades por segundo)",
         "WASD-Kamerageschwindigkeit (Einheiten pro Sekunde)",
         "Vitesse de la caméra WASD (unités par seconde)",
         "WASD カメラ速度（単位/秒）",
         "WASD 카메라 속도(초당 단위)",
         "Velocidade da câmera WASD (unidades por segundo)"}},
    {"Автобэкап",       {"Автобэкап", "Auto backup", "自动备份", "نسخ احتياطي تلقائي",
                        "Copia automática", "Auto-Backup", "Sauvegarde auto",
                        "自動バックアップ", "자동 백업", "Backup automático"}},
    {"Раз в минуту сцена сохраняется в папку backup рядом с программой",
        {"Раз в минуту сцена сохраняется в папку backup рядом с программой",
         "The scene is saved every minute to the backup folder next to the program",
         "每分钟将场景保存到程序旁的 backup 文件夹",
         "يُحفظ المشهد كل دقيقة في مجلد backup بجوار البرنامج",
         "La escena se guarda cada minuto en la carpeta backup junto al programa",
         "Die Szene wird jede Minute im Ordner backup neben dem Programm gespeichert",
         "La scène est sauvegardée chaque minute dans le dossier backup à côté du programme",
         "シーンは毎分、プログラム横の backup フォルダに保存されます",
         "장면은 매 분 프로그램 옆의 backup 폴더에 저장됩니다",
         "A cena é salva a cada minuto na pasta backup ao lado do programa"}},

    {"Аутлайнер",       {"Аутлайнер", "Outliner", "大纲视图", "الهيكل",
                        "Outliner", "Outliner", "Outliner",
                        "アウトライナー", "아웃라이너", "Outliner"}},
    {"Объекты",         {"Объекты", "Objects", "对象", "الكائنات",
                        "Objetos", "Objekte", "Objets",
                        "オブジェクト", "개체", "Objetos"}},
    {"Свойства",        {"Свойства", "Properties", "属性", "الخصائص",
                        "Propiedades", "Eigenschaften", "Propriétés",
                        "プロパティ", "속성", "Propriedades"}},
    {"Имя",             {"Имя", "Name", "名称", "الاسم",
                        "Nombre", "Name", "Nom",
                        "名前", "이름", "Nome"}},
    {"Позиция",         {"Позиция", "Position", "位置", "الموضع",
                        "Posición", "Position", "Position",
                        "位置", "위치", "Posição"}},
    {"Поворот",         {"Поворот", "Rotation", "旋转", "الدوران",
                        "Rotación", "Rotation", "Rotation",
                        "回転", "회전", "Rotação"}},
    {"Масштаб",         {"Масштаб", "Scale", "缩放", "التحجيم",
                        "Escala", "Skalierung", "Échelle",
                        "スケール", "크기", "Escala"}},
    {"Материал",        {"Материал", "Material", "材质", "الخامة",
                        "Material", "Material", "Matériau",
                        "マテリアル", "재질", "Material"}},
    {"Текстура: нет",   {"Текстура: нет", "Texture: none", "纹理：无", "النسيج: لا يوجد",
                        "Textura: ninguna", "Textur: keine", "Texture : aucune",
                        "テクスチャ：なし", "텍스처: 없음", "Textura: nenhuma"}},

    {"Готово",          {"Готово", "Ready", "就绪", "جاهز",
                        "Listo", "Bereit", "Prêt",
                        "準備完了", "준비됨", "Pronto"}},
    {"Тема: светлая",   {"Тема: светлая", "Theme: light", "主题：浅色", "السمة: فاتحة",
                        "Tema: claro", "Design: hell", "Thème : clair",
                        "テーマ：ライト", "테마: 밝게", "Tema: claro"}},
    {"Тема: тёмная",    {"Тема: тёмная", "Theme: dark", "主题：深色", "السمة: داكنة",
                        "Tema: oscuro", "Design: dunkel", "Thème : sombre",
                        "テーマ：ダーク", "테마: 어둡게", "Tema: escuro"}},
    {"Фон сцены",       {"Фон сцены", "Scene background", "场景背景", "خلفية المشهد",
                        "Fondo de escena", "Szenenhintergrund", "Fond de scène",
                        "シーン背景", "장면 배경", "Fundo da cena"}},
    {"Фон сцены: светлый", {"Фон сцены: светлый", "Scene background: light",
                           "场景背景：浅色", "خلفية المشهد: فاتحة",
                           "Fondo de escena: claro", "Szenenhintergrund: hell",
                           "Fond de scène : clair", "シーン背景：ライト",
                           "장면 배경: 밝게", "Fundo da cena: claro"}},
    {"Фон сцены: тёмный",  {"Фон сцены: тёмный", "Scene background: dark",
                           "场景背景：深色", "خلفية المشهد: داكنة",
                           "Fondo de escena: oscuro", "Szenenhintergrund: dunkel",
                           "Fond de scène : sombre", "シーン背景：ダーク",
                           "장면 배경: 어둡게", "Fundo da cena: escuro"}},

    {"Тема интерфейса", {"Тема интерфейса", "Interface theme", "界面主题", "سمة الواجهة",
                        "Tema de la interfaz", "Oberflächendesign",
                        "Thème de l'interface", "インターフェーステーマ",
                        "인터페이스 테마", "Tema da interface"}},
    {"Светлая",         {"Светлая", "Light", "浅色", "فاتح",
                        "Clara", "Hell", "Clair",
                        "ライト", "밝게", "Clara"}},
    {"Тёмная",          {"Тёмная", "Dark", "深色", "داكن",
                        "Oscura", "Dunkel", "Sombre",
                        "ダーク", "어둡게", "Escura"}},
    {"Светлый",         {"Светлый", "Light", "浅色", "فاتح",
                        "Claro", "Hell", "Clair",
                        "ライト", "밝게", "Claro"}},
    {"Тёмный",          {"Тёмный", "Dark", "深色", "داكن",
                        "Oscuro", "Dunkel", "Sombre",
                        "ダーク", "어둡게", "Escuro"}},
    {"Свой цвет...",    {"Свой цвет...", "Custom color...", "自定义颜色...", "لون مخصص...",
                        "Color personalizado...", "Eigene Farbe...",
                        "Couleur personnalisée...", "カスタムカラー...",
                        "사용자 지정 색상...", "Cor personalizada..."}},
    {"Выбрать цвет",    {"Выбрать цвет", "Pick color", "选择颜色", "اختر لونًا",
                        "Elegir color", "Farbe wählen", "Choisir une couleur",
                        "色を選択", "색상 선택", "Escolher cor"}},
    {"Язык",            {"Язык", "Language", "语言", "اللغة",
                        "Idioma", "Sprache", "Langue",
                        "言語", "언어", "Idioma"}},
    {"Язык применится после перезапуска",
        {"Язык применится после перезапуска",
         "Language change takes effect after restart",
         "语言将在重启后生效",
         "تسري تغييرات اللغة بعد إعادة التشغيل",
         "El idioma se aplicará tras reiniciar",
         "Die Sprache wird nach einem Neustart wirksam",
         "La langue sera appliquée après redémarrage",
         "言語の変更は再起動後に有効になります",
         "언어 변경은 재시작 후 적용됩니다",
         "O idioma será aplicado após reiniciar"}},
};

inline QString Tr(const char* key) {
    for (const TrEntry& e : kTable)
        if (qstrcmp(e.key, key) == 0)
            return QString::fromUtf8(pick(e.v));
    return QString::fromUtf8(key);
}

} // namespace Localization
