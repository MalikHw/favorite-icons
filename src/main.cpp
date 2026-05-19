#include <Geode/Geode.hpp>
#include <Geode/modify/GJGarageLayer.hpp>

using namespace geode::prelude;

static const char* SAVE_KEY = "favourites";
struct FavEntry { int type; int id; };
static std::vector<FavEntry> loadFavourites() {
    auto arr = Mod::get()->getSavedValue<matjson::Value>(SAVE_KEY, matjson::Value::array());
    std::vector<FavEntry> result;
    if (!arr.isArray()) return result;
    static std::vector<matjson::Value> empty{};
    for (auto& v : arr.asArray().unwrapOr(empty)) {
        if (!v.isObject()) continue;
        auto t = v["type"].asInt();
        auto i = v["id"].asInt();
        if (t && i) result.push_back({t.unwrap(), i.unwrap()});
    }
    return result;
}
static void saveFavourites(const std::vector<FavEntry>& favs) {
    auto arr = matjson::Value::array();
    for (auto& e : favs) {
        auto obj = matjson::Value::object();
        obj["type"] = e.type;
        obj["id"] = e.id;
        arr.asArray().unwrap().push_back(obj);
    }
    Mod::get()->setSavedValue(SAVE_KEY, arr);
}
static bool isFavourite(int type, int id) {
    for (auto& e : loadFavourites())
        if (e.type == type && e.id == id) return true;
    return false;
}
static void addFavourite(int type, int id) {
    auto favs = loadFavourites();
    for (auto& e : favs)
        if (e.type == type && e.id == id) return;
    favs.push_back({type, id});
    saveFavourites(favs);
}
static void removeFavourite(int type, int id) {
    auto favs = loadFavourites();
    favs.erase(std::remove_if(favs.begin(), favs.end(),
        [&](const FavEntry& e){ return e.type == type && e.id == id; }),
        favs.end());
    saveFavourites(favs);
}
class HeartButton : public CCMenuItemSpriteExtra {
public:
    int m_iconType;
    int m_iconId;
    bool m_isFav;
    static HeartButton* create(int iconType, int iconId, CCObject* target, SEL_MenuHandler callback) {
        bool fav = isFavourite(iconType, iconId);
        auto spr = CCSprite::createWithSpriteFrameName(fav ? "gj_heartOn_001.png" : "gj_heartOff_001.png");
        spr->setScale(0.55f);
        auto ret = new HeartButton();
        ret->m_iconType = iconType;
        ret->m_iconId = iconId;
        ret->m_isFav = fav;
        if (ret->CCMenuItemSpriteExtra::init(spr, nullptr, target, callback)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
    void syncSprite() {
        auto spr = CCSprite::createWithSpriteFrameName(m_isFav ? "gj_heartOn_001.png" : "gj_heartOff_001.png");
        spr->setScale(0.55f);
        this->setNormalImage(spr);
    }
};
class $modify(MyGarageLayer, GJGarageLayer) {
    struct Fields {
        bool m_onFavsPage = false;
        CCMenu* m_heartMenu = nullptr;
        CCNode* m_favsPageNode = nullptr;
        CCMenuItemToggler* m_favsTabBtn = nullptr;
    };
    void updateFavsTabSprite() {
        auto btn = m_fields->m_favsTabBtn;
        if (!btn) return;
        bool hasFavs = !loadFavourites().empty();
        auto spr = CCSprite::createWithSpriteFrameName(hasFavs ? "gj_heartOn_001.png" : "gj_heartOff_001.png");
        spr->setScale(1.2f);
        btn->m_offButton->setNormalImage(spr);
    }
    void buildFavsPage() {
        auto& f = m_fields;
        if (f->m_favsPageNode) { f->m_favsPageNode->removeFromParent(); f->m_favsPageNode = nullptr; }
        if (f->m_heartMenu) { f->m_heartMenu->removeFromParent(); f->m_heartMenu = nullptr; }
        auto favs = loadFavourites();
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        auto page = CCNode::create();
        page->setID("favs-page"_spr);
        this->addChild(page, 10);
        f->m_favsPageNode = page;
        auto heartMenu = CCMenu::create();
        heartMenu->setID("favs-heart-menu"_spr);
        heartMenu->setTouchPriority(-504);
        this->addChild(heartMenu, 11);
        f->m_heartMenu = heartMenu;
        if (favs.empty()) {
            auto label = CCLabelBMFont::create("No favourites yet!\nPress the heart under any icon.", "bigFont.fnt");
            label->setAlignment(kCCTextAlignmentCenter);
            label->setScale(0.45f);
            label->setPosition(winSize.width * 0.5f, winSize.height * 0.5f);
            page->addChild(label);
            return;
        }
        const float startX = 115.f, startY = 265.f, cellW = 43.f, cellH = 46.f;
        const int cols = 8;
        for (int i = 0; i < (int)favs.size(); i++) {
            auto& e = favs[i];
            float cx = startX + (i % cols) * cellW;
            float cy = startY - (i / cols) * cellH;
            auto unlockType = GameManager::get()->iconTypeToUnlockType(static_cast<IconType>(e.type));
            auto icon = GJItemIcon::createBrowserItem(unlockType, e.id);
            if (!icon) icon = GJItemIcon::createStoreItem(unlockType, e.id, false, ccc3(255,255,255));
            if (icon) { icon->setPosition(ccp(cx, cy + 6.f)); icon->setScale(0.8f); page->addChild(icon); }
            auto heart = HeartButton::create(e.type, e.id, this, menu_selector(MyGarageLayer::onHeartPressed));
            heart->setPosition(ccp(cx, cy - 14.f));
            heart->setTag(i);
            heartMenu->addChild(heart);
        }
    }
    void showVanillaPage() {
        auto& f = m_fields;
        f->m_onFavsPage = false;
        if (f->m_favsPageNode) { f->m_favsPageNode->removeFromParent(); f->m_favsPageNode = nullptr; }
        if (f->m_heartMenu) { f->m_heartMenu->removeFromParent(); f->m_heartMenu = nullptr; }
        if (auto ic = this->getChildByID("icon-selection")) ic->setVisible(true);
        if (auto lbb = typeinfo_cast<CCNode*>(m_iconSelection)) lbb->setVisible(true);
        if (auto arrows = this->getChildByID("arrows-menu")) arrows->setVisible(true);
        if (auto navDots = m_navDotMenu) navDots->setVisible(true);
    }
    void hideVanillaPage() {
        if (auto ic = this->getChildByID("icon-selection")) ic->setVisible(false);
        if (auto lbb = typeinfo_cast<CCNode*>(m_iconSelection)) lbb->setVisible(false);
        if (auto arrows = this->getChildByID("arrows-menu")) arrows->setVisible(false);
        if (auto navDots = m_navDotMenu) navDots->setVisible(false);
    }
    bool init() {
        if (!GJGarageLayer::init()) return false;
        addFavsTabButton();
        addHeartButtonsToCurrentPage();
        return true;
    }
    void addFavsTabButton() {
        CCMenu* tabMenu = nullptr;
        for (int i = 0; i < this->getChildrenCount(); i++) {
            auto child = typeinfo_cast<CCMenu*>(this->getChildren()->objectAtIndex(i));
            if (!child || child->getChildrenCount() < 3) continue;
            if (typeinfo_cast<CCMenuItemToggler*>(child->getChildren()->objectAtIndex(0))) {
                tabMenu = child;
                break;
            }
        }
        if (!tabMenu) { log::warn("FavIcons: could not find tab menu"); return; }
        tabMenu->setID("tab-menu"_spr);
        bool hasFavs = !loadFavourites().empty();
        auto offSpr = CCSprite::createWithSpriteFrameName(hasFavs ? "gj_heartOn_001.png" : "gj_heartOff_001.png");
        offSpr->setScale(1.2f);
        auto onSpr = CCSprite::createWithSpriteFrameName("gj_heartOn_001.png");
        onSpr->setScale(1.2f);
        auto toggler = CCMenuItemToggler::create(offSpr, onSpr, this, menu_selector(MyGarageLayer::onFavsTab));
        toggler->setID("favs-tab-btn"_spr);
        float lastX = 0.f;
        for (int i = 0; i < (int)tabMenu->getChildrenCount(); i++) {
            auto child = typeinfo_cast<CCNode*>(tabMenu->getChildren()->objectAtIndex(i));
            if (child) lastX = std::max(lastX, child->getPositionX() + child->getContentSize().width * child->getScaleX() * 0.5f);
        }
        float tabY = tabMenu->getChildrenCount() > 0
            ? typeinfo_cast<CCNode*>(tabMenu->getChildren()->objectAtIndex(0))->getPositionY()
            : 0.f;
        toggler->setPosition(ccp(lastX + 26.f, tabY));
        tabMenu->addChild(toggler);
        m_fields->m_favsTabBtn = toggler;
    }
    void addHeartButtonsToCurrentPage() {
        if (m_fields->m_heartMenu) { m_fields->m_heartMenu->removeFromParent(); m_fields->m_heartMenu = nullptr; }
        if (m_fields->m_onFavsPage) return;
        auto heartMenu = CCMenu::create();
        heartMenu->setID("heart-menu"_spr);
        heartMenu->setTouchPriority(-504);
        this->addChild(heartMenu, 10);
        m_fields->m_heartMenu = heartMenu;
        if (!m_iconSelection) return;
        auto lbb = m_iconSelection;
        if (!lbb->m_pages || lbb->m_pages->count() == 0) return;
        int curPage = 0;
        auto it = m_iconPages.find(m_iconType);
        if (it != m_iconPages.end()) curPage = it->second;
        if (curPage < 0 || curPage >= (int)lbb->m_pages->count()) curPage = 0;
        auto pageNode = typeinfo_cast<CCNode*>(lbb->m_pages->objectAtIndex(curPage));
        if (!pageNode) return;
        CCMenu* iconMenu = nullptr;
        for (int i = 0; i < (int)pageNode->getChildrenCount(); i++) {
            auto child = typeinfo_cast<CCMenu*>(pageNode->getChildren()->objectAtIndex(i));
            if (child && child->getChildrenCount() > 0) { iconMenu = child; break; }
        }
        if (!iconMenu) return;
        int iconType = (int)m_iconType;
        for (int i = 0; i < (int)iconMenu->getChildrenCount(); i++) {
            auto item = typeinfo_cast<CCMenuItemSpriteExtra*>(iconMenu->getChildren()->objectAtIndex(i));
            if (!item) continue;
            auto layerPos = this->convertToNodeSpace(iconMenu->convertToWorldSpace(item->getPosition()));
            auto heart = HeartButton::create(iconType, item->getTag(), this, menu_selector(MyGarageLayer::onHeartPressed));
            heart->setPosition(ccp(layerPos.x, layerPos.y - 18.f));
            heartMenu->addChild(heart);
        }
    }
    void onFavsTab(CCObject* sender) {
        for (int i = 0; i < (int)m_tabButtons->count(); i++) {
            auto t = typeinfo_cast<CCMenuItemToggler*>(m_tabButtons->objectAtIndex(i));
            if (t) t->toggle(false);
        }
        if (!m_fields->m_onFavsPage) {
            m_fields->m_onFavsPage = true;
            hideVanillaPage();
            buildFavsPage();
        } else {
            showVanillaPage();
            if (m_tabButtons && m_tabButtons->count() > 0)
                if (auto firstTab = typeinfo_cast<CCMenuItemToggler*>(m_tabButtons->objectAtIndex(0)))
                    firstTab->toggle(true);
            this->selectTab(IconType::Cube);
            if (auto btn = typeinfo_cast<CCMenuItemToggler*>(sender)) btn->toggle(false);
        }
    }
    void onHeartPressed(CCObject* sender) {
        auto heart = typeinfo_cast<HeartButton*>(sender);
        if (!heart) return;
        if (heart->m_isFav) {
            removeFavourite(heart->m_iconType, heart->m_iconId);
            heart->m_isFav = false;
            heart->syncSprite();
            if (m_fields->m_onFavsPage) buildFavsPage();
        } else {
            addFavourite(heart->m_iconType, heart->m_iconId);
            heart->m_isFav = true;
            heart->syncSprite();
        }
        updateFavsTabSprite();
    }
    void selectTab(IconType type) {
        GJGarageLayer::selectTab(type);
        if (m_fields->m_onFavsPage) {
            m_fields->m_onFavsPage = false;
            showVanillaPage();
            if (m_fields->m_favsTabBtn) m_fields->m_favsTabBtn->toggle(false);
        }
        this->scheduleOnce(schedule_selector(MyGarageLayer::refreshHearts), 0.f);
    }
    void listButtonBarSwitchedPage(ListButtonBar* bar, int page) {
        GJGarageLayer::listButtonBarSwitchedPage(bar, page);
        this->scheduleOnce(schedule_selector(MyGarageLayer::refreshHearts), 0.f);
    }
    void refreshHearts(float) {
        addHeartButtonsToCurrentPage();
    }
};