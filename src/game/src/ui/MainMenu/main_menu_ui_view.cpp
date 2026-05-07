#include "ui/MainMenu/main_menu_ui_view.h"
#include "ui_data_mainmenu.h"
#include "bn_string.h"

MainMenuUIView::MainMenuUIView(UIManager& ui_manager)
    : ui_manager_(ui_manager) {
    for (int i = 0; i < 5; ++i) {
        unlocked_flags_[i] = true;
        text_nodes_[i] = nullptr;
        paper_images_[i] = nullptr;
    }
}

void MainMenuUIView::init(const bool unlocked_flags[5], int initial_cursor) {
    // 1. UIの静的データをロードして画面を構築
    ui_manager_.load_screen(ui_data_mainmenu::SCREEN);

    // 解禁状況をView内でも保持しておく（選択更新時に使うため）
    for (int i = 0; i < 5; ++i) {
        unlocked_flags_[i] = unlocked_flags[i];
    }

    // 2. 頻繁に更新するノードのポインタを取得してキャッシュ
    text_nodes_[0] = ui_manager_.get_text("storymode_text");
    text_nodes_[1] = ui_manager_.get_text("practicemode_text");
    text_nodes_[2] = ui_manager_.get_text("endlessmode_text");
    text_nodes_[3] = ui_manager_.get_text("gallerymode_text");
    text_nodes_[4] = ui_manager_.get_text("settingmode_text");

    paper_images_[0] = ui_manager_.get_image("storymode_paper");
    paper_images_[1] = ui_manager_.get_image("practicemode_paper");
    paper_images_[2] = ui_manager_.get_image("endlessmode_paper_copy");
    paper_images_[3] = ui_manager_.get_image("gallerymode_paper_copy");
    paper_images_[4] = ui_manager_.get_image("settingmode_paper_copy");

    // 3. 解禁状況に応じた見た目の初期化（Viewの責務）
    for(int i = 0; i < 5; ++i) {
        if (!unlocked_flags_[i]) {
            // 未解禁なら画像を暗いもの（例：image_no = 1）にする
            if(paper_images_[i]) {
                ui_manager_.change_sprite_image(paper_images_[i], "ui_paper", 1);
            }
        }
    }

    // 4. 初期カーソルの反映
    update_selection(initial_cursor);
}

void MainMenuUIView::update() {
    // View固有の毎フレーム処理（独自の揺らぎ等）があればここに記述
    
    // UIエンジンの更新を回す
    ui_manager_.update();
}

void MainMenuUIView::update_selection(int cursor_index) {
    // ベースとなるテキスト文字列（Viewが知っていれば良い情報）
    const char* base_texts[] = {
        "ストーリーモード", // 0: STORY
        "プラクティス",     // 1: PRACTICE
        "エンドレス",       // 2: ENDLESS
        "ギャラリー",       // 3: GALLERY
        "設定"              // 4: SETTINGS
    };

    // Stateから渡されたインデックスに基づき、見た目を更新
    for (int i = 0; i < 5; ++i) {
        if (!text_nodes_[i]) continue;

        bn::string<32> display_text;
        
        // 選択中項目の装飾
        if (i == cursor_index) {
            display_text.append("> ");
        }

        // 未解禁の場合はテキストを伏せる等の処理
        if (!unlocked_flags_[i]) {
            display_text.append("？？？");
        } else {
            display_text.append(base_texts[i]);
        }

        text_nodes_[i]->set_text(display_text);
    }
}

void MainMenuUIView::play_exit_animation() {
    // 画面からハケるアニメーションを再生する（アニメーションIDはJSONの定義に合わせる）
    if(auto* anim = ui_manager_.get_anim("pop_out")) {
        anim->play();
    }
}