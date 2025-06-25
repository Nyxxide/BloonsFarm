#include "BloonsUIMain.h"

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
    this->setWindowIcon(QIcon("../resources/UI/btdfarmicon.ico"));
    auto *window = new QWidget();
    auto *layout = new QVBoxLayout();
    auto *mainhboxtop = new QHBoxLayout();
    vector<QHBoxLayout*> mainhboxbotrows;
    auto *runninghbox = new QHBoxLayout();
    auto *runningvbox = new QVBoxLayout();
    auto *runninglabelhbox = new QHBoxLayout();
    auto *runningbuttonhbox = new QHBoxLayout();

    // Font setup
    auto font_id = QFontDatabase::addApplicationFont("../resources/UI/LuckiestGuy-Regular.ttf");
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
                connect(editcoords_action, &QAction::triggered, this, &BloonsUIMain::editcoords);
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
                connect(startloopbutton, &QPushButton::clicked, this, &BloonsUIMain::startloop);
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
    connect(quitButton, &QPushButton::clicked, this, &BloonsUIMain::endloop);

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
void BloonsUIMain::editcoords() {
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

void BloonsUIMain::startloop() {
    for(const auto& button : farmButtonList){
        button->hide();
    }
    mainLabel->hide();
    activeLabel->show();
    quitButton->show();
}

void BloonsUIMain::endloop() {
    for(const auto& button : farmButtonList){
        button->show();
    }
    mainLabel->show();
    activeLabel->hide();
    quitButton->hide();
}