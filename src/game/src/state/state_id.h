#ifndef STATE_ID_H
#define STATE_ID_H

enum class StateID {
    NONE = -1,
    TITLE = 0,
    MENU,
    EVENT,
    PUZZLE,
    SETTINGS,
    PRACTICE,
    ENDLESS,
    GALLERY,
    DEBUG_MENU,
    // --- ここより上に追加していく ---
    COUNT // 登録されているStateの総数（配列のサイズとして使用）
};

#endif // STATE_ID_H