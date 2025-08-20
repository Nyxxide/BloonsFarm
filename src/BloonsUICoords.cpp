#include "BloonsUICoords.h"

int newx = 0;
int newy = 0;

void hook_callback(uiohook_event* const event) {
    if (event->type == EVENT_MOUSE_PRESSED) {
        newx = event->data.mouse.x;
        newy = event->data.mouse.y;
        hook_stop();
    }
}

// Function for formatting key for json
std::string formatKey(const std::string& label) {
    std::string key = label;
    transform(key.begin(), key.end(), key.begin(), ::tolower);
    replace(key.begin(), key.end(), ' ', '_');
    return key;
}

// Function for updating the json files
void updateJson(std::string buttonText, std::string fileName){
    std::string filePath = "Tower Positions/" + fileName + ".json";

    std::ifstream inFile(filePath);
    if (!inFile.is_open()) {
        auto* error = new BloonsUIPopup("No File Found", "Error! Coordinate file not found!");
        error->show();
        return;
    }

    nlohmann::json j;
    inFile >> j;
    inFile.close();

    std::string key = formatKey(buttonText);

    j["towers"][key]["x"] = newx;
    j["towers"][key]["y"] = newy;

    std::ofstream outFile(filePath, std::ios::trunc);
    if (!outFile.is_open()) {
        auto* error = new BloonsUIPopup("File Write Error", "Error! Could not write to coordinate file!");
        error->show();
        return;
    }

    outFile << j.dump(4);
}

BloonsUICoords::BloonsUICoords(std::string name, std::vector<std::string> labels, std::string file) {
    // Pass in class variables from constructor
    title = name;
    button_labels = labels;
    fileName = file;

    // Window setup
    this->setWindowTitle(QString::fromStdString(title));
    this->setWindowIcon(QIcon(":/resources/UI/btdfarmicon.ico"));
    this->setGeometry(550, 250, 400, 300);
    this->setWindowModality(Qt::WindowModality::ApplicationModal);

    // Layout setup
    auto *sub = new QWidget();
    auto *outvbox = new QVBoxLayout();
    auto *sublayout = new QHBoxLayout();
    auto *sublayout2 = new QHBoxLayout();
    auto *buttonvbox = new QVBoxLayout();
    auto *labelvbox = new QVBoxLayout();

    // Font setup
    auto font_id = QFontDatabase::addApplicationFont(":/resources/UI/LuckiestGuy-Regular.ttf");
    auto font_name = QFontDatabase::applicationFontFamilies(font_id)[0];
    auto BTDFont = QFont(font_name);

    // Subwindow Label setup
    subwinlab = new QLabel("Select an option to change its position:");
    BTDFont.setPointSize(15);
    subwinlab->setFont(BTDFont);
    subwinlab2 = new QLabel("Click where you want the new tower position to be");
    BTDFont.setPointSize(13);
    subwinlab2->setFont(BTDFont);
    subwinlab2->hide();

    // Setup buttons for all towers
    for(const auto& label : button_labels){
        auto *button = new QPushButton(QString::fromStdString(label), this);
        button->setFixedSize(200,50);
        BTDFont.setPointSize(13);
        button->setFont(BTDFont);
        connect(button, &QPushButton::clicked, this, &BloonsUICoords::resetpos);
        buttonvbox->addWidget(button);
        buttons.push_back(button);
    }

    // Assemble Layout
    auto *padding = new QWidget();
    labelvbox->addWidget(subwinlab);
    labelvbox->addWidget(subwinlab2);
    sublayout->addWidget(padding);
    sublayout->addLayout(labelvbox);
    sublayout->addWidget(padding);
    sublayout2->addWidget(padding);
    sublayout2->addLayout(buttonvbox);
    sublayout2->addWidget(padding);
    outvbox->addLayout(sublayout);
    outvbox->addLayout(sublayout2);
    sub->setLayout(outvbox);
    this->setCentralWidget(sub);
}

void BloonsUICoords::resetpos() {// Store button that called this function
    QObject *sender = QObject::sender();
    QPushButton *button = qobject_cast<QPushButton*>(sender);
    if(button){
        QString buttonmsg = button->text();
        std::string buttonText = buttonmsg.toStdString();

        // Hide existing layout
        subwinlab->hide();
        subwinlab2->show();
        for(const auto& towerbutton : buttons){
            towerbutton->hide();
        }
        QApplication::processEvents();

        // Watch for mouse click and store position data
        hook_set_dispatch_proc(hook_callback);
        if (hook_run() != UIOHOOK_SUCCESS) {
            auto* error = new BloonsUIPopup("Broken Library", "Fatal Error! Broken Library! (not something you can fix, reach out to me)");
            error->show();
            return;
        }

        // Rewrite json data
        updateJson(buttonText, fileName);

        // Show popup window
        auto* success = new BloonsUIPopup("Success", "Position successfully changed!");
        success->show();

        // Show hidden layout
        subwinlab->show();
        subwinlab2->hide();
        for(const auto& towerbutton : buttons){
            towerbutton->show();
        }
    }
}