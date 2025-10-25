#include "UIManager.hpp"

void UIManager::handleUIEvent(const UIEvent& e) {
    switch (e.type) {
        case UIEventType::ChangeScreen:
            showScreen(e.target);
            break;
        case UIEventType::Popup:
            //showPopup(e.target);
            break;
        case UIEventType::Toast:
            //showToast(e.target);
            break;
        default:
            break;
    }
}