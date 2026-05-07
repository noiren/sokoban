#ifndef MAIN_MENU_UI_VIEW_H
#define MAIN_MENU_UI_VIEW_H

#include "ui/Core/Manager/ui_manager.h"

// 画面の見た目（View）を専任で管理するクラス
class MainMenuUIView {
public:
    MainMenuUIView(UIManager& ui_manager);

    // 解禁状況(bool配列)と初期カーソルを受け取って画面を構築する
    void init(const bool unlocked_flags[5], int initial_cursor);

    // 毎フレームの更新（UIManagerのupdateをラップしつつ固有のView処理を行う）
    void update();

    // Stateから「論理的なカーソル位置(0〜4)」を受け取り、見た目に反映する
    void update_selection(int cursor_index);

    // 画面遷移する時に呼ばれる「ハケる」演出のトリガー
    void play_exit_animation();

private:
    UIManager& ui_manager_;
    
    // View側で表示状態を保持しておくためのフラグ
    bool unlocked_flags_[5];

    // 操作頻度の高いテキスト部品・画像部品をキャッシュしておく
    // インデックスは MenuItem Enum の順（STORY, PRACTICE, ENDLESS, GALLERY, SETTINGS）に対応
    UIText* text_nodes_[5];
    UIImage* paper_images_[5];
};

#endif // MAIN_MENU_UI_VIEW_H