#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "UI/button.hpp"
#include "UI/textbox.hpp"
#include "UI/label.hpp"
#include "UI/ui_state.hpp"
#include "UI/color_selector.hpp"
#include "UI/ui_element.hpp"

// ==================== 固定 UI View（800x600） ====================
sf::View uiView(sf::FloatRect(0, 0, UI_WIDTH, UI_HEIGHT));

// ==================== 全域背景 ====================
sf::Texture g_bgTex;
sf::Sprite  g_bgSprite;
sf::RectangleShape g_bgOverlay;
bool g_bgLoaded = false;

// 在「UI 座標空間」(800x600) 裡做等比例縮放
void updateBackgroundUI()
{
    float uiW = UI_WIDTH;
    float uiH = UI_HEIGHT;

    float imgW = g_bgTex.getSize().x;
    float imgH = g_bgTex.getSize().y;

    if (imgW == 0 || imgH == 0) return;

    float scale = std::max(uiW / imgW, uiH / imgH);
    g_bgSprite.setScale(scale, scale);

    float posX = (uiW - imgW * scale) / 2.f;
    float posY = (uiH - imgH * scale) / 2.f;
    g_bgSprite.setPosition(posX, posY);

    g_bgOverlay.setSize({uiW, uiH});
}

void initBackground()
{
    if (g_bgLoaded) return;
    g_bgLoaded = true;

    g_bgTex.loadFromFile("assets/cat_bg.png");
    g_bgSprite.setTexture(g_bgTex);
    g_bgOverlay.setFillColor(sf::Color(0, 0, 0, 100));

    updateBackgroundUI();
}

// ==================== 房間資料 ====================

struct Room {
    int id;

    std::vector<std::string> playerNames; // 真實玩家名單，index 0 = Host
    int maxPlayers;                       // 3/4/5，0 = 尚未設定（空房）

    bool isPrivate;
    std::string password;                // private 時 4 位數
    bool inGame;

    std::string name;

    int players() const { return static_cast<int>(playerNames.size()); }
    bool hasSettings() const { return maxPlayers != 0; }
    bool isFull() const { return hasSettings() && players() >= maxPlayers; }

    std::string hostName() const {
        return playerNames.empty() ? "" : playerNames[0];
    }

    void resetIfEmpty()
    {
        if (!playerNames.empty()) return;
        maxPlayers = 0;
        isPrivate  = false;
        password.clear();
        inGame = false;
    }
};

// 初始假資料：
// Room1: Host Alice, 1/3 public
// Room2: Host Bob, Carol, 2/5 private, key 1234
// Room3: 空房，尚未設定
std::vector<Room> rooms = {
    {1, {"Alice"},           3, false, "",     false, "Room 1"},
    {2, {"Bob", "Carol"},    5, true,  "1234", false, "Room 2"},
    {3, {},                  0, false, "",     false, "Room 3"}
};

int currentRoomIndex = -1;

enum class EndReason {
    None,
    RoomsFull,
    UserExit,
    WrongKeyTooMany,
    Timeout
};

// ==================== Username Page ====================

void runUsernamePage(sf::RenderWindow& window, State& state,
                     std::string& username, EndReason& reason)
{
    sf::Font font;
    font.loadFromFile("fonts/NotoSans-Regular.ttf");

    Label title(&font, "Cat In The Sack", 180, 70, 56,
                sf::Color::White, sf::Color::Black, 4);

    sf::RectangleShape panel({500, 300});
    panel.setFillColor(sf::Color(255,255,255,220));
    panel.setOutlineColor(sf::Color(100,100,100));
    panel.setOutlineThickness(4);
    panel.setPosition(150, 150);

    Label enterUser(&font, "Enter Username", 260, 160, 32,
                    sf::Color::White, sf::Color::Black, 3);

    TextBox usernameBox(&font, 250, 230, 300, 55);
    Button okBtn(&font, "START", 310, 330, 180, 65);
    Button exitBtn(&font, "Exit", 650, 20, 120, 40);

    while (window.isOpen() && state == State::UsernameInput)
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            usernameBox.handleEvent(event, window);

            if (okBtn.clicked(event, window)) {
                username = usernameBox.buffer;
                if (!username.empty())
                    state = State::RoomInfo;
            }

            if (exitBtn.clicked(event, window)) {
                reason = EndReason::UserExit;
                state  = State::EndConn;
            }
        }

        okBtn.update(window);
        exitBtn.update(window);

        window.setView(uiView);
        window.clear();

        window.draw(g_bgSprite);
        window.draw(g_bgOverlay);

        window.draw(panel);
        title.draw(window);
        enterUser.draw(window);
        usernameBox.draw(window);
        okBtn.draw(window);
        exitBtn.draw(window);

        window.display();
    }
}

// ==================== Host Setting Page ====================
// 只會在「空房」第一個玩家進來時出現
void runHostSettingPage(sf::RenderWindow& window, State& state,
                        EndReason& /*reason*/, Room& room,
                        const std::string& username)
{
    sf::Font font;
    font.loadFromFile("fonts/NotoSans-Regular.ttf");

    Label title(&font, "Room Settings", 220, 40, 46,
                sf::Color::White, sf::Color::Black, 4);

    std::string hostStr = "Host: " + username;
    Label hostLabel(&font, hostStr, 220, 100, 26,
                    sf::Color::White, sf::Color::Black, 2);

    // 玩家數選擇：按鈕 & 字體都縮小，不超出畫面
    float bw = 200.f, bh = 60.f;
    Button max3(&font, "3 Players", 80,  160, bw, bh);
    Button max4(&font, "4 Players", 300, 160, bw, bh);
    Button max5(&font, "5 Players", 520, 160, bw, bh);

    max3.text.setCharacterSize(24);
    max4.text.setCharacterSize(24);
    max5.text.setCharacterSize(24);
    for (Button* b : {&max3, &max4, &max5}) {
        sf::FloatRect tb = b->text.getLocalBounds();
        b->text.setPosition(
            b->shape.getPosition().x + (b->shape.getSize().x - tb.width)/2.f - tb.left,
            b->shape.getPosition().y + (b->shape.getSize().y - tb.height)/2.f - tb.top
        );
    }

    int chosenMax = 0;

    // Public / Private 往下移一點，不和 Players 列重疊
    Button publicBtn (&font, "Public", 230, 240, 180, 60);
    Button privateBtn(&font, "Private", 430, 240, 180, 60);
    publicBtn.text.setCharacterSize(28);
    privateBtn.text.setCharacterSize(28);
    for (Button* b : {&publicBtn, &privateBtn}) {
        sf::FloatRect tb = b->text.getLocalBounds();
        b->text.setPosition(
            b->shape.getPosition().x + (b->shape.getSize().x - tb.width)/2.f - tb.left,
            b->shape.getPosition().y + (b->shape.getSize().y - tb.height)/2.f - tb.top
        );
    }

    bool isPrivate = false;

    // 密碼（private 時）
    const int PASS_LEN = 4;
    std::string pwBuf;
    sf::RectangleShape pwBox[PASS_LEN];
    for (int i = 0; i < PASS_LEN; i++) {
        pwBox[i].setSize({50, 60});
        pwBox[i].setOutlineThickness(3);
        pwBox[i].setOutlineColor(sf::Color::Black);
        pwBox[i].setFillColor(sf::Color(255,255,255,230));
    }

    // 說明文字：擺在密碼格上方
    Label pwLabel(&font, "Password (4 digits)", 230, 320, 22,
                  sf::Color::White, sf::Color::Black, 2);

    Button confirmBtn(&font, "CONFIRM", 300, 470, 200, 70);

    std::string errorMsg;

    while (window.isOpen() && state == State::HostSetting)
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            // 人數選擇
            if (max3.clicked(event, window)) chosenMax = 3;
            if (max4.clicked(event, window)) chosenMax = 4;
            if (max5.clicked(event, window)) chosenMax = 5;

            // Public / Private
            if (publicBtn.clicked(event, window)) {
                isPrivate = false;
                pwBuf.clear();
            }
            if (privateBtn.clicked(event, window)) {
                isPrivate = true;
            }

            // 密碼輸入
            if (isPrivate && event.type == sf::Event::TextEntered) {
                if (event.text.unicode == 8) {          // Backspace
                    if (!pwBuf.empty()) pwBuf.pop_back();
                } else if (event.text.unicode >= '0' &&
                           event.text.unicode <= '9') { // digit
                    if (pwBuf.size() < PASS_LEN)
                        pwBuf.push_back((char)event.text.unicode);
                }
            }

            // 確認
            if (confirmBtn.clicked(event, window)) {
                if (chosenMax == 0) {
                    errorMsg = "Please choose player count.";
                } else if (isPrivate && pwBuf.size() != PASS_LEN) {
                    errorMsg = "Password must be 4 digits.";
                } else {
                    room.maxPlayers = chosenMax;
                    room.isPrivate  = isPrivate;
                    room.password   = isPrivate ? pwBuf : "";
                    room.inGame     = false;
                    // Host 已在 playerNames[0]
                    state = State::InRoom;
                }
            }
        }

        // hover 效果
        max3.update(window);
        max4.update(window);
        max5.update(window);
        publicBtn.update(window);
        privateBtn.update(window);
        confirmBtn.update(window);

        window.setView(uiView);
        window.clear();

        window.draw(g_bgSprite);
        window.draw(g_bgOverlay);

        title.draw(window);
        hostLabel.draw(window);

        auto setChoiceColor = [](Button& btn, bool chosen) {
            btn.shape.setFillColor(chosen ? sf::Color(200,240,200)
                                          : sf::Color(220,220,220));
        };
        setChoiceColor(max3, chosenMax == 3);
        setChoiceColor(max4, chosenMax == 4);
        setChoiceColor(max5, chosenMax == 5);

        max3.draw(window);
        max4.draw(window);
        max5.draw(window);

        publicBtn.shape.setFillColor(!isPrivate ? sf::Color(200,240,200)
                                                : sf::Color(220,220,220));
        privateBtn.shape.setFillColor(isPrivate ? sf::Color(200,240,200)
                                                : sf::Color(220,220,220));
        publicBtn.draw(window);
        privateBtn.draw(window);

        // 密碼區：只有 private 時顯示
        if (isPrivate) {
            pwLabel.draw(window);
            for (int i = 0; i < PASS_LEN; i++) {
                pwBox[i].setPosition(230 + i * 60.f, 350);
                window.draw(pwBox[i]);

                if (i < (int)pwBuf.size()) {
                    sf::Text digit(std::string(1, pwBuf[i]), font, 32);
                    digit.setFillColor(sf::Color::Black);
                    digit.setPosition(240 + i * 60.f, 348);
                    window.draw(digit);
                }
            }
        }

        if (!errorMsg.empty()) {
            sf::Text e(errorMsg, font, 20);
            e.setFillColor(sf::Color::Red);
            e.setPosition(230, 410);
            window.draw(e);
        }

        confirmBtn.draw(window);

        window.display();
    }
}


// ==================== RoomInfo Page ====================

void runRoomInfoPage(sf::RenderWindow& window, State& state,
                     EndReason& reason, const std::string& username)
{
    sf::Font font;
    font.loadFromFile("fonts/NotoSans-Regular.ttf");

    Label title(&font, "Select a Room", 200, 40, 50,
                sf::Color::White, sf::Color::Black, 4);

    Button exitBtn(&font, "Exit", 650, 20, 120, 40);

    Button roomBtn[3] = {
        Button(&font, "Room 1", 120, 140, 360, 80),
        Button(&font, "Room 2", 120, 260, 360, 80),
        Button(&font, "Room 3", 120, 380, 360, 80)
    };

    const int PASS_LEN = 4;
    std::string keyInput;
    sf::RectangleShape passBox[PASS_LEN];
    for (int i = 0; i < PASS_LEN; i++) {
        passBox[i].setSize({40, 50});
        passBox[i].setFillColor(sf::Color(255,255,255,230));
        passBox[i].setOutlineColor(sf::Color::Black);
        passBox[i].setOutlineThickness(3);
    }

    // 密碼說明字直接放在右邊、密碼格上方
    Label passLabel(&font, "Password (4 digits)", 520, 205, 22,
                    sf::Color::White, sf::Color::Black, 2);

    Button joinBtn(&font, "JOIN", 520, 360, 150, 60);

    Label selectedLabel(&font, "", 500, 150, 24,
                        sf::Color::White, sf::Color::Black, 3);

    int  selected      = -1;
    bool needsKey      = false;
    std::string errorMsg;
    int  wrongKeyCount = 0;

    sf::Clock timer;

    while (window.isOpen() && state == State::RoomInfo)
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            // 離開整個 lobby
            if (exitBtn.clicked(event, window)) {
                reason = EndReason::UserExit;
                state  = State::EndConn;
            }

            auto canClick = [&](int i) {
                Room& r = rooms[i];
                if (r.inGame) return false;
                if (r.hasSettings() && r.isFull()) return false;
                return true;
            };

            // 選房間
            for (int i = 0; i < 3; i++) {
                if (canClick(i) && roomBtn[i].clicked(event, window)) {
                    selected = i;
                    Room& r = rooms[i];
                    needsKey = r.isPrivate && r.hasSettings();
                    keyInput.clear();
                    errorMsg.clear();
                    selectedLabel.text.setString("Selected: " + r.name);
                }
            }

            // 密碼輸入
            if (needsKey && selected != -1 &&
                event.type == sf::Event::TextEntered)
            {
                if (event.text.unicode >= '0' && event.text.unicode <= '9') {
                    if (keyInput.size() < PASS_LEN)
                        keyInput.push_back(static_cast<char>(event.text.unicode));
                }
                if (event.text.unicode == 8 && !keyInput.empty())
                    keyInput.pop_back();
            }

            // JOIN
            if (selected != -1 && joinBtn.clicked(event, window))
            {
                Room& r = rooms[selected];

                if (r.inGame || (r.hasSettings() && r.isFull())) {
                    errorMsg = "Room is full.";
                    continue;
                }

                if (r.playerNames.empty()) {
                    // 空房：成為房主，先 HostSetting
                    r.playerNames.push_back(username);
                    currentRoomIndex = selected;
                    state = State::HostSetting;
                } else {
                    // 需要密碼
                    if (r.isPrivate && r.hasSettings()) {
                        if (keyInput != r.password) {
                            wrongKeyCount++;
                            errorMsg = "Wrong password!";
                            keyInput.clear();
                            if (wrongKeyCount >= 3) {
                                reason = EndReason::WrongKeyTooMany;
                                state  = State::EndConn;
                            }
                            continue;
                        }
                    }
                    // 加入房間（避免重複）
                    if (std::find(r.playerNames.begin(),
                                  r.playerNames.end(),
                                  username) == r.playerNames.end())
                    {
                        r.playerNames.push_back(username);
                    }
                    currentRoomIndex = selected;
                    state = State::InRoom;
                }
            }
        }

        // 60 秒 timeout
        float remain = 60.f - timer.getElapsedTime().asSeconds();
        if (remain <= 0.f) {
            reason = EndReason::Timeout;
            state  = State::EndConn;
        }

        // ===== Render =====
        exitBtn.update(window);
        for (int i = 0; i < 3; i++)
            roomBtn[i].update(window);
        joinBtn.update(window);

        window.setView(uiView);
        window.clear();

        window.draw(g_bgSprite);
        window.draw(g_bgOverlay);

        title.draw(window);

        // Timer
        sf::Text timerText("Time left: " + std::to_string((int)remain) + "s",
                           font, 26);
        timerText.setFillColor(sf::Color::White);
        timerText.setOutlineColor(sf::Color::Black);
        timerText.setOutlineThickness(2);
        timerText.setPosition(300, 520);
        window.draw(timerText);

        exitBtn.draw(window);

        // 三間房資訊
        for (int i = 0; i < 3; i++)
        {
            Room&   r = rooms[i];
            Button& b = roomBtn[i];

            bool full = r.hasSettings() && r.isFull();

            sf::Color base(210,230,250);
            if (full)      base = sf::Color(180,180,180);
            if (r.inGame)  base = sf::Color(255,215,180);
            if (selected == i) base = sf::Color(255,240,140);

            b.shape.setFillColor(base);
            b.text.setString("");
            b.draw(window);

            // Host
            std::string hostName = r.hostName();
            sf::Text hostTx(hostName.empty() ? "Host: -" : "Host: " + hostName,
                            font, 20);
            hostTx.setFillColor(sf::Color::Black);
            hostTx.setPosition(b.shape.getPosition().x + 12,
                               b.shape.getPosition().y + 8);
            window.draw(hostTx);

            // Room name
            sf::Text roomName(r.name, font, 32);
            roomName.setFillColor(sf::Color::Black);
            roomName.setPosition(b.shape.getPosition().x + 130,
                                 b.shape.getPosition().y + 25);
            window.draw(roomName);

            // 人數 or Empty
            sf::Text st("", font, 22);
            st.setFillColor(sf::Color::Black);
            if (r.playerNames.empty() && !r.hasSettings()) {
                st.setString("Empty room");
            } else if (r.hasSettings()) {
                bool fullNow = r.isFull();
                st.setString(std::to_string(r.players()) + " / " +
                             std::to_string(r.maxPlayers) +
                             (fullNow ? " (Full)" : ""));
            } else {
                st.setString(std::to_string(r.players()) + " players");
            }
            st.setPosition(b.shape.getPosition().x + 12,
                           b.shape.getPosition().y + 55);
            window.draw(st);

            // PRIVATE 標籤
            if (r.isPrivate) {
                sf::RectangleShape tag({80,22});
                tag.setFillColor(sf::Color(255,200,200));
                tag.setOutlineColor(sf::Color(180,30,30));
                tag.setOutlineThickness(2);
                tag.setPosition(b.shape.getPosition().x + b.shape.getSize().x - 90,
                                b.shape.getPosition().y + 28);
                window.draw(tag);

                sf::Text tx("PRIVATE", font, 14);
                tx.setFillColor(sf::Color(180,30,30));
                tx.setPosition(tag.getPosition().x + 8,
                               tag.getPosition().y + 2);
                window.draw(tx);
            }
        }

        // 右側選中 + 密碼區
        if (selected != -1)
        {
            Room& rsel = rooms[selected];

            selectedLabel.draw(window);

            // Host 顯示
            std::string h = rsel.hostName();
            if (!h.empty()) {
                sf::Text hostInfo("Host: " + h, font, 20);
                hostInfo.setFillColor(sf::Color::White);
                hostInfo.setOutlineColor(sf::Color::Black);
                hostInfo.setOutlineThickness(2);
                hostInfo.setPosition(500, 180);
                window.draw(hostInfo);
            }

            // JOIN 按鈕
            joinBtn.draw(window);

            // 密碼 UI：標題在上，四格在下
            if (needsKey) {
                passLabel.draw(window);

                for (int i = 0; i < PASS_LEN; i++) {
                    passBox[i].setPosition(520 + i * 55.f, 240);
                    window.draw(passBox[i]);

                    if (i < (int)keyInput.size()) {
                        sf::Text digit(std::string(1, keyInput[i]), font, 32);
                        digit.setFillColor(sf::Color::Black);
                        digit.setPosition(528 + i * 55.f, 238);
                        window.draw(digit);
                    }
                }
            }

            // 錯誤訊息：在密碼區下方，不擋 JOIN
            if (!errorMsg.empty()) {
                sf::Text e(errorMsg, font, 20);
                e.setFillColor(sf::Color::Red);
                e.setPosition(520, 305);
                window.draw(e);
            }
        }

        window.display();
    }
}


// ==================== InRoom Page ====================

// ==================== InRoom Page ====================

void runInRoomPage(sf::RenderWindow &window, State &state,
                   const std::string &username, EndReason &reason)
{
    sf::Font font;
    font.loadFromFile("fonts/NotoSans-Regular.ttf");

    int roomIdx = currentRoomIndex;
    Room &room = rooms[roomIdx];

    auto &players = room.playerNames;
    int n = players.size();

    int myIndex = 0;
    for (int i = 0; i < n; i++)
        if (players[i] == username) myIndex = i;

    std::vector<bool> isReady(n, false);
    std::vector<int>  colorIndex(n, -1);

    bool isHost = (players[0] == username);

    Label title(&font, "Room Lobby", 240, 40, 52,
                sf::Color::White, sf::Color::Black, 5);

    float panelHeight = 80 + n * 60;
    sf::RectangleShape listPanel({420, panelHeight});
    listPanel.setPosition(40, 150);
    listPanel.setFillColor(sf::Color(255,255,255,210));
    listPanel.setOutlineColor(sf::Color(120,120,120));
    listPanel.setOutlineThickness(4);

    Button exitBtn(&font, "Exit", 650, 20, 120, 40);
    Button settingBtn(&font, "SETTING", 500, 330, 200, 55);
    settingBtn.text.setCharacterSize(26);

    Button readyBtn(&font, "READY", 500, 400, 200, 70);
    Button startBtn(&font, "START GAME", 40, 480, 260, 70);
    startBtn.text.setCharacterSize(26);

    Label colorLabel(&font, "Choose your color:", 500, 170, 28,
                     sf::Color::White, sf::Color::Black, 3);

    ColorSelector selector(0,0);
    selector.setLimit(room.maxPlayers);
    selector.computePositions(640, 260);

    Label readyStateLabel(&font, "Not Ready", 540, 500, 22,
                          sf::Color::White, sf::Color::Black, 2);

    // ======================
    // ★ Auto-kick 邏輯
    // ======================
    sf::Clock idleTimer;
    const float KICK_TIME = 30.f;  // ✔ 秒數統一管理
    bool counting = true;    // true = 倒數中，false = 停止倒數（Ready 時）

    auto resetIdle = [&]() { idleTimer.restart(); counting = true; };
    auto stopIdle  = [&]() { counting = false; };

    auto isColorTaken = [&](int col){
        for (int i = 0; i < n; i++)
            if (i != myIndex && colorIndex[i] == col)
                return true;
        return false;
    };

    while (window.isOpen() && state == State::InRoom)
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (exitBtn.clicked(event, window))
            {
                auto it = std::find(players.begin(), players.end(), username);
                if (it != players.end()) players.erase(it);

                if (players.empty())
                    room.resetIfEmpty();

                state = State::RoomInfo;
                return;
            }

            // ========== Ready toggle（強制要求選顏色）==========
            if (readyBtn.clicked(event, window))
            {
                // ⛔ 玩家沒有選顏色 → 不允許 ready
                if (colorIndex[myIndex] == -1) 
                {
                    // 你也可以加個紅色提醒文字
                    // 例如 readyError = "Please choose a color first!";
                    std::cout << "Ready denied: choose color first\n";
                    continue; // ❗ 非常重要：不要繼續往下執行 toggle！
                }

                // ⭐ 已選顏色 → 允許 ready / not ready
                isReady[myIndex] = !isReady[myIndex];

                if (isReady[myIndex]) {
                // ✔ 按下 READY → 停止倒數
                counting = false;
            } else {
                // ✔ 按下 NOT READY → 重新開始倒數
                counting = true;
                idleTimer.restart();
            }
            }


            if (!isReady[myIndex])
            {
                selector.updateClick(event, window,
                    [&](int c){ return isColorTaken(c); });
                colorIndex[myIndex] = selector.selected;
            }

            if (isHost && settingBtn.clicked(event, window))
            {
                state = State::HostSetting;
                return;
            }

            // ====== Start Game 按鈕（只能在滿人+全 ready 按）======
            if (isHost && startBtn.clicked(event, window))
            {
                bool allReady = true;
                for (bool r : isReady) if (!r) allReady = false;

                bool roomFull = (players.size() == room.maxPlayers);

                if (allReady && roomFull)
                {
                    state = State::GameStart;
                    return;
                }
            }
        }

        // ======================
        // ★ Auto-kick 執行
        // ======================
        if (counting)
        {
            float elapsed = idleTimer.getElapsedTime().asSeconds();
            if (elapsed > KICK_TIME)
            {
                auto it = std::find(players.begin(), players.end(), username);
                if (it != players.end()) players.erase(it);

                state = State::RoomInfo;
                return;
            }
        }

        // ===== Draw =====
        window.setView(uiView);
        window.clear();
        window.draw(g_bgSprite);
        window.draw(g_bgOverlay);

        title.draw(window);
        window.draw(listPanel);

        // ===== 玩家列表 =====
        for (int i = 0; i < n; i++)
        {
            std::string nm = players[i];
            if (i == 0) nm += " (Host)";
            if (i == myIndex) nm += " [You]";

            sf::Text pname(nm, font, 26);
            pname.setFillColor(colorIndex[i] >= 0 ?
                               PLAYER_COLORS[colorIndex[i]] :
                               sf::Color::Black);

            float nameX = 70;
            float nameY = 160 + i * 60;
            pname.setPosition(nameX, nameY);

            // 避免名字擋到 Ready
            float maxRight = 230.f;
            float allowedWidth = maxRight - nameX;

            while (pname.getLocalBounds().width > allowedWidth &&
                   pname.getCharacterSize() > 12)
            {
                pname.setCharacterSize(pname.getCharacterSize() - 1);
            }

            window.draw(pname);

            sf::Text r(isReady[i] ? "Ready" : "Not Ready", font, 24);
            r.setFillColor(isReady[i] ? sf::Color(0,150,0)
                                      : sf::Color(150,0,0));
            r.setPosition(250, 165 + i * 60);
            window.draw(r);
        }

        colorLabel.draw(window);

        selector.preview.setFillColor(
            colorIndex[myIndex] >= 0 ?
            PLAYER_COLORS[colorIndex[myIndex]] :
            sf::Color(200,200,200)
        );
        window.draw(selector.preview);
        selector.draw(window, isColorTaken);

        exitBtn.update(window);
        exitBtn.draw(window);

        readyBtn.shape.setFillColor(
            isReady[myIndex] ?
            sf::Color(170,220,170) :
            sf::Color(220,220,220)
        );
        readyBtn.update(window);
        readyBtn.draw(window);

        readyStateLabel.text.setString(
            isReady[myIndex] ? "You are READY" : "Not Ready"
        );
        readyStateLabel.draw(window);

        // ===== Host 按鈕 =====
        if (isHost)
        {
            settingBtn.update(window);
            settingBtn.draw(window);

            bool allReady = true;
            for (bool r : isReady) if (!r) allReady = false;
            bool roomFull = (players.size() == room.maxPlayers);
            bool canStart = allReady && roomFull;

            // ⭐ Start Game 一律深灰除非 canStart
            startBtn.shape.setFillColor(
                canStart ? sf::Color(120,200,120)
                         : sf::Color(30,30,30)
            );

            startBtn.update(window);
            startBtn.draw(window);
        }

        // ====== Idle countdown（只有 Not Ready 才顯示）======
        if (!isReady[myIndex])
        {
            float remain = KICK_TIME - idleTimer.getElapsedTime().asSeconds();
            if (remain < 0) remain = 0;

            sf::Text idleText("Auto-kick in: " + std::to_string((int)remain) + "s",
                              font, 22);
            idleText.setFillColor(sf::Color::White);
            idleText.setOutlineThickness(2);
            idleText.setOutlineColor(sf::Color::Black);
            idleText.setPosition(320, 560);
            window.draw(idleText);
        }

        window.display();
    }
}




// ==================== End Connection Page ====================

void runEndConnPage(sf::RenderWindow& window, State& state, EndReason reason)
{
    sf::Font font;
    font.loadFromFile("fonts/NotoSans-Regular.ttf");

    std::string msg;
    switch (reason) {
        case EndReason::RoomsFull:
            msg = "All rooms are full.\nConnection closed."; break;
        case EndReason::UserExit:
            msg = "You exited the lobby.\nConnection closed."; break;
        case EndReason::WrongKeyTooMany:
            msg = "Too many wrong passwords.\nConnection closed."; break;
        case EndReason::Timeout:
            msg = "Timeout: You did not join a room in 60s.\nConnection closed."; break;
        default:
            msg = "Connection ended."; break;
    }

    Button okBtn(&font, "OK", 330, 350, 160, 60);

    while (window.isOpen() && state == State::EndConn)
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (okBtn.clicked(event, window))
                window.close();
        }

        okBtn.update(window);

        window.setView(uiView);
        window.clear();

        window.draw(g_bgSprite);
        window.draw(g_bgOverlay);

        sf::RectangleShape panel({520, 260});
        panel.setPosition(140, 170);
        panel.setFillColor(sf::Color(255,255,255,240));
        panel.setOutlineColor(sf::Color(180,180,180));
        panel.setOutlineThickness(3);
        window.draw(panel);

        Label title(&font, "Disconnected", 240, 190, 40,
                    sf::Color::White, sf::Color::Black, 4);
        title.draw(window);

        sf::Text tx(msg, font, 24);
        tx.setFillColor(sf::Color::Black);
        tx.setPosition(170, 240);
        window.draw(tx);

        okBtn.draw(window);

        window.display();
    }
}

// ==================== MAIN ====================

int main()
{
    initBackground();

    sf::RenderWindow window(sf::VideoMode(800, 600),
                            "Cat In The Sack",
                            sf::Style::Default);
    window.setVerticalSyncEnabled(true);

    State state = State::UsernameInput;
    EndReason reason = EndReason::None;
    std::string username;

    while (window.isOpen())
    {
        switch (state)
        {
            case State::UsernameInput:
                runUsernamePage(window, state, username, reason);
                break;

            case State::RoomInfo:
                runRoomInfoPage(window, state, reason, username);
                break;

            case State::HostSetting:
                if (currentRoomIndex >= 0 &&
                    currentRoomIndex < static_cast<int>(rooms.size()))
                {
                    runHostSettingPage(window, state, reason,
                                       rooms[currentRoomIndex], username);
                }
                else {
                    state = State::RoomInfo;
                }
                break;

            case State::InRoom:
                runInRoomPage(window, state, username, reason);
                break;

            case State::EndConn:
                runEndConnPage(window, state, reason);
                break;
        }
    }

    return 0;
}
