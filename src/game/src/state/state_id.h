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
    STORY,        // ストーリー進行管理（画面なし）
    STILL_EVENT,  // スチルイベント（全画面イラスト＋テキスト）
    // --- ここより上に追加していく ---
    COUNT // 登録されているStateの総数（配列のサイズとして使用）
};

#endif // STATE_ID_H