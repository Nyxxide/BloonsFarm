#include "BloonsUIMain.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <time.h>

void printCurrentEST() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t now_c = system_clock::to_time_t(now);

    // Convert to EST (Eastern Standard Time)
    std::tm est_tm = *std::localtime(&now_c);
    std::cout << "[DEBUG] Current EST time: "
              << std::put_time(&est_tm, "%Y-%m-%d %H:%M:%S") << std::endl;
}


BloonsUIMain::BloonsUIMain(){


    //TODO: setup the logic loop (Use OpenCV and Battery/embed)

    // Init app to get screen resolution
    QApplication MainUI(int argc = 0, char** empty = nullptr);

    // Thread setup to gen data if necessary
    std::thread t(genData);
    t.join();

    // Setup and show the main window (Defaults Full Screen)
    this->setWindowTitle("BloonsFarm++");
    this->setGeometry(550, 250, 800, 600);
    this->setWindowIcon(QIcon(":/resources/UI/btdfarmicon.ico"));
    auto *window = new QWidget();
    auto *layout = new QVBoxLayout();
    auto *mainhboxtop = new QHBoxLayout();
    vector<QHBoxLayout*> mainhboxbotrows;
    auto *runninghbox = new QHBoxLayout();
    auto *runningvbox = new QVBoxLayout();

    // Font setup
    auto font_id = QFontDatabase::addApplicationFont(":/resources/UI/LuckiestGuy-Regular.ttf");
    auto font_name = QFontDatabase::applicationFontFamilies(font_id)[0];
    auto BTDFont = QFont(font_name);

    // Add main window label
    mainLabel = new QLabel("Choose your desired farming method");
    mainLabel->setFixedSize(600, 50);
    BTDFont.setPointSize(24);
    mainLabel->setFont(BTDFont);

    // Add farm-active label
    activeLabel = new QLabel("");
    BTDFont.setPointSize(17);
    activeLabel->setFont(BTDFont);
    activeLabel->setAlignment(Qt::AlignCenter);
    activeLabel->hide();

    // Add a menu bar
    auto *menubar = new QMenuBar();
    auto *editmenu = new QMenu("Edit", menubar);

    // Add menu options to menu bar
    try{
        for(const auto& file : directory_iterator("Tower Positions")){
            if(file.path().extension() == ".json"){
                string fixedName = replaceChar(file.path(), '_', ' ');
                fixedName = removeSubstring(fixedName, "Tower Positions/");
                fixedName = toTitleCase(fixedName);
                fixedName = removeSubstring(fixedName, ".json");
                QString temp = QString::fromStdString(fixedName);
                auto *editcoords_action = new QAction(temp);
                editmenu->addAction(editcoords_action);
                connect(editcoords_action, &QAction::triggered, this, &BloonsUIMain::editCoords);
            }
        }
    }
    catch(const filesystem_error &err){
        auto *error = new BloonsUIPopup("Error", "Files could not be opened!");
        error->show();
    }

    // Add edit option to menubar
    menubar->addMenu(editmenu);

    // Farm Buttons setup
    int buttonnum = 0;
    int buttonrow = -1;

    try{
        for(const auto& file : directory_iterator("Tower Positions")){
            if(file.path().extension() == ".json"){
                if(buttonnum % 4 == 0){
                    mainhboxbotrows.push_back(new QHBoxLayout);
                    buttonrow += 1;
                }
                string fixedName = replaceChar(file.path(), '_', ' ');
                fixedName = removeSubstring(fixedName, "Tower Positions/");
                fixedName = toTitleCase(fixedName);
                fixedName = removeSubstring(fixedName, ".json");
                QString temp = QString::fromStdString(fixedName);
                auto *startloopbutton = new QPushButton(temp, this);
                startloopbutton->setFixedSize(200, 100);
                BTDFont.setPointSize(20);
                startloopbutton->setFont(BTDFont);
                setMaxFontSize(startloopbutton, 20);
                connect(startloopbutton, &QPushButton::clicked, this, &BloonsUIMain::startLoop);
                mainhboxbotrows[buttonrow]->addWidget(startloopbutton);
                farmButtonList.push_back(startloopbutton);
                buttonnum += 1;
            }
        }
    }
    catch(const filesystem_error &err){
        auto *error = new BloonsUIPopup("Error", "Files could not be opened!");
        error->show();
    }

    // Quit Button setup
    quitButton = new QPushButton("Quit to Menu");
    quitButton->hide();
    BTDFont.setPointSize(20);
    quitButton->setFont(BTDFont);
    quitButton->setFixedSize(200, 100);
    connect(quitButton, &QPushButton::clicked, this, &BloonsUIMain::endLoop);

    // Add Widgets and setup layout of main UI
    mainhboxtop->addWidget(mainLabel);
    layout->setAlignment(mainhboxtop, Qt::AlignCenter);
    layout->addLayout(mainhboxtop);
    for(const auto& row : mainhboxbotrows){
        layout->addLayout(row);
    }

    // Add Widgets and setup layout of active farm UI
    runningvbox->addWidget(activeLabel);
    runningvbox->addWidget(quitButton);
    runningvbox->setAlignment(activeLabel, Qt::AlignCenter);
    runningvbox->setAlignment(quitButton, Qt::AlignCenter);
    runninghbox->addLayout(runningvbox);
    layout->addLayout(runninghbox);

    // Finalize UI setup and open the window
    window->setLayout(layout);
    this->setCentralWidget(window);
    this->setMenuBar(menubar);
    this->show();
};

// Function to edit tower coordinates
void BloonsUIMain::editCoords() {
    QObject *sender = QObject::sender();
    QAction *action = qobject_cast<QAction*>(sender);
    if(action){
        QString menuoption = action->text();
        string strmenuopt = menuoption.toStdString();
        menuoption = menuoption.toLower().replace(" ", "_");
        string strmenucoded = menuoption.toStdString();
        ifstream jsonFile("Tower Positions/" + menuoption.toStdString() + ".json");
        json data;
        jsonFile >> data;
        jsonFile.close();
        vector<string> buttonlabels;
        for(json::iterator towerName = data["towers"].begin(); towerName != data["towers"].end(); ++towerName){
            string temp = replaceChar(towerName.key(), '_', ' ');
            temp = toTitleCase(temp);
            buttonlabels.push_back(temp);
        }
        auto *defsubwin = new BloonsUICoords(strmenuopt, buttonlabels, strmenucoded);
        defsubwin->show();
        defsubwin->raise();
    }
}

void BloonsUIMain::setMaxFontSize(QPushButton* button, double maxFontSizePt) {
    if (!button) return;                         // safety

    QFont font = button->font();
    font.setPointSizeF(maxFontSizePt);           // start size
    QFontMetricsF fm(font);

    const QString text  = button->text();
    const int     width = button->width();
    const int     height = button->height();

    // Shrink until both the bounding-box width and the line height fit.
    while ((fm.boundingRect(text).width()  > width) ||
           (fm.height()                    > height))
    {
        maxFontSizePt -= 1.3;                   // same decrement as Python
        if (maxFontSizePt < 1.0) break;        // hard floor; avoid zero / negative

        font.setPointSizeF(maxFontSizePt);
        fm = QFontMetricsF(font);
    }

    button->setFont(font);
}

void BloonsUIMain::menuNav(string fileName) {
    // 1. Load JSON
    std::ifstream f("Tower Positions/" + fileName + ".json");
    if (!f) { std::cerr << "Cannot open JSON\n"; return; }
    json data = json::parse(f);

    const auto& nv  = data["menuNav"];
    const std::string mapDifficulty = nv["mapDifficulty"];
    const std::string map           = nv["map"];
    const std::string difficulty    = nv["difficulty"];
    const std::string mode          = nv["mode"];

    QString mapPath = QString(QString::fromStdString(":/resources/Maps/" + map + ".png"));
    QString mapDiffpath = QString(QString::fromStdString(":/resources/MapDifficulty/" + mapDifficulty + ".png"));
    QString diffPath= QString(QString::fromStdString(":/resources/Difficulty/" + difficulty + ".png"));
    QString modePath = QString(QString::fromStdString(":/resources/Mode/" + mode + ".png"));

    // 2. Attempt to locate map on screen
    auto mapMatch = waitForTemplate(mapPath, 0.7, 3, 250, &running);
    cout << "Checking for map on screen" << endl;
    if(!running) return;

    // 3. If not visible, look for difficulty
    if (!mapMatch){
        while(running){
            auto mapDiffMatch = waitForTemplate(mapDiffpath, 0.7, -1, 250, &running);
            cout << "Attempting to find map difficulty." << endl;
            if(!running) return;
            if(mapDiffMatch){
                clickCenter(mapDiffMatch->bbox);
                mapMatch = waitForTemplate(mapPath, 0.7, 5, 250);
                if(mapMatch) break;
            }
        }
    }
    if(!running) return;

    // 4. Click Map
    clickCenter(mapMatch->bbox, 20);
    cout << "Found Map" << endl;
    if(!running) return;

    // 5. Locate and Click Difficulty
    auto diffMatch = waitForTemplate(diffPath, 0.7, -1, 250, &running);
    if(!running) return;
    cout << diffMatch->bbox << endl;
    clickCenter(diffMatch->bbox);
    if(!running) return;
    cout << "Found Difficulty" << endl;


    // 6. Locate and Click Difficulty
    auto modeMatch = waitForTemplate(modePath, 0.7, -1, 250, &running);
    if(!running) return;
    clickCenter(modeMatch->bbox);
    cout << "Found Mode" << endl;
}

void BloonsUIMain::towerPlacement(string fileName) {
    // load JSON
    std::ifstream in("Tower Positions/" + fileName + ".json");
    if (!in) { qWarning("Cannot open JSON file"); return; }

    nlohmann::json j;
    in >> j;

    // Loop through towers
    for (auto& [towerName, node] : j["towers"].items())
    {
        char hotkey  = node["hotkey"].get<std::string>()[0];
        int  x       = node["x"];
        int  y       = node["y"];
        int  top     = node["top"];
        int  middle  = node["middle"];
        int  bottom  = node["bottom"];

        clickAt(x, y);
        msleep(250);

        pressChar(hotkey);
        msleep(250);

        clickAt(x, y);
        msleep(250);
        clickAt(x, y);

        for (int i = 0; i < top;    ++i) { msleep(250); pressSpecial(","); }
        for (int i = 0; i < middle; ++i) {  msleep(250); pressSpecial("."); }
        for (int i = 0; i < bottom; ++i) { msleep(250); pressSpecial("/"); }

        msleep(250);
    }
}

void BloonsUIMain::farmLoop() {



    auto findAndClick = [this](const QString& resPath,
                               double thresh,
                               int    maxTries = -1,
                               int    delayMs  = 50,
                               int    yOffset  = 0) -> void
    {
        auto m = waitForTemplate(resPath, thresh, maxTries, delayMs, &running);
        if(!running) return;
        clickCenter(m->bbox, yOffset);
    };

    while(running){
        // 1. Home menu
        findAndClick(":/resources/MenuNav/homemenu.png", 0.7, -1, 250);
        cout << "We Home Menu" << endl;
        if(!running) break;

        // 2. Navigate Map Menus
        menuNav(activeFile);
        if(!running) break;
        cout << "We Menu Nav" << endl;

        // 3. Look for pre-existing game
        if(waitForTemplate(":/resources/MenuNav/existinggame.png", 0.7, 5, 250, &running)){
            findAndClick(":/resources/MenuNav/existinggameok.png", 0.7, -1, 250);
            cout << "We PreExisting Game" << endl;
        }

        // 4. Wait for in game HUD
        waitForTemplate(":/resources/MenuNav/ingame.png", 0.7, -1, 250);
        cout << "We Find In Game HUD" << endl;

        // 5. Tooltip suppression
        if (waitForTemplate(":/resources/MenuNav/deflationtooltip.png", 0.70, 5, 250, &running)){
            cout << "We Tooltip Menu" << endl;
            findAndClick(":/resources/MenuNav/tooltipok.png", 0.70, -1, 250);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // 6. Tower Placement
        towerPlacement(activeFile);
        if (!running) break;
        cout << "We Place Towers" << endl;

        // 7. Start Round
        pressSpecial("space");
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        pressSpecial("space");
        pressSpecial("esc");
        cout << "We Start Round" << endl;

        // 8. Wait for Game End or Level-up Interrupt
        while (running)
        {
            // Game End
            cout << "Checking for end\n" << endl;
            printCurrentEST();
            auto endBtn = waitForTemplate(":/resources/MenuNav/endnext.png", 0.70, 1, 250, &running);
            if (endBtn) {
                clickCenter(endBtn->bbox);
                cout << "We End Game" << endl;
                break;
            }
            // Level-up Interrupt
            cout << "Checking for level\n" << endl;
            if (auto lvl = waitForTemplate(":/resources/MenuNav/levelup.png", 0.70, 1, 250, &running))
            {   clickCenter(lvl->bbox); clickCenter(lvl->bbox); }
        }
        if (!running) break;

        // 9. Go Home
        findAndClick(":/resources/MenuNav/endhome.png", 0.80);
        cout << "We Go Home" << endl;

        // 10. Collection Event Watch
        auto coll = waitForTemplate(":/resources/MenuNav/collectionevent.png", 0.70, 10, 250, &running);
        if (coll && running)
        {
            cout << "We Collection Menu" << endl;
            clickCenter(coll->bbox);
            findAndClick(":/resources/MenuNav/instamonkey.png", 0.70);
            cout << "We InstaMonkey" << endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(800));
            clickCenter(coll->bbox);   // confirm once
            std::this_thread::sleep_for(std::chrono::milliseconds(800));


            while (running)
            {
                if (auto endcollection = waitForTemplate(":/resources/MenuNav/endcollection.png", 0.70, 5, 250)) {
                    clickCenter(endcollection->bbox);
                    break;
                }
                auto insta = waitForTemplate(":/resources/MenuNav/instamonkey.png", 0.70, 5, 250, &running);
                cout << "We InstaMonkeyLoop" << endl;
                clickCenter(insta->bbox);
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
                clickCenter(insta->bbox);
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
            }
            cout << "We End Collection" << endl;

            findAndClick(":/resources/MenuNav/collectionback.png", 0.70);
            cout << "We collection back" << endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
    }

}

void BloonsUIMain::startLoop() {
    for(const auto& button : farmButtonList){
        button->hide();
    }
    mainLabel->hide();
    activeLabel->show();
    quitButton->show();
    QObject *sender = QObject::sender();
    QPushButton *button = qobject_cast<QPushButton*>(sender);
    if(button){
        QString buttonmsg = button->text();
        string buttonText = buttonmsg.toStdString();
        activeLabel->setText(QString::fromStdString("Program is currently running " + buttonText + " farm"));
        activeFile = replaceChar(buttonText, ' ', '_');
        transform(activeFile.begin(), activeFile.end(), activeFile.begin(), ::tolower);
    }

    running = true;

    loopThread = std::thread([this] {farmLoop();});
}

void BloonsUIMain::endLoop() {
    running = false;

    if (loopThread.joinable()) {
        loopThread.join();
    }

    for(const auto& button : farmButtonList){
        button->show();
    }
    mainLabel->show();
    activeLabel->hide();
    quitButton->hide();
    activeLabel->setText(QString::fromStdString(""));
    activeFile = "";
}