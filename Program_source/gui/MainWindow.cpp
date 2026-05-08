#include "MainWindow.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include "../filesystem/SharedFolder.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QStandardPaths>
#include <QGroupBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QComboBox>
#include <fstream>
#include <algorithm>

MainWindow::MainWindow(Folder* root, const std::string& username, FileManager& fm, AuthService& auth, QWidget* parent)
    : QMainWindow(parent), m_root(root), m_currentUser(username), m_fm(fm), m_auth(auth),
    m_model(nullptr), m_searchEngine(), m_pathResolver(nullptr), m_logger(nullptr),
    m_currentSort(SortManager::SortCrit::NAME_ASC), m_treeView(nullptr), m_previewPane(nullptr),
    m_searchBar(nullptr), m_addressBar(nullptr), m_searchResults(nullptr), m_mainSplitter(nullptr),
    m_statusLabel(nullptr), m_logConsole(nullptr)
{
    m_pathResolver = new PathResolver(std::shared_ptr<Folder>(root, [](Folder*){}));
    m_logger = new TimestampedLogger("logs.txt");

    setWindowTitle(QString("ATMosFS — %1").arg(QString::fromStdString(username)));
    setMinimumSize(1024, 680);
    setupMenuBar();
    setupUI();

    logEvent("LOGIN", username);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSystemState();
    logEvent("LOGOUT", m_currentUser);
    QMainWindow::closeEvent(event);
}

void MainWindow::saveSystemState() {
    logEvent("SYSTEM", "Datele au fost salvate automat in baza de date SQLite.");
}

void MainWindow::loadSystemState() {
    logEvent("SYSTEM", "Sistemul a fost incarcat cu succes din SQLite.");
}

void MainWindow::setupMenuBar() {
    QMenu* fileMenu      = menuBar()->addMenu("File");
    QAction* newFileAction   = fileMenu->addAction("Fisier nou");
    QAction* newFolderAction = fileMenu->addAction("Folder nou");
    fileMenu->addSeparator();
    QAction* exitAction  = fileMenu->addAction("Iesire");

    QMenu* editMenu       = menuBar()->addMenu("Edit");
    QAction* renameAction = editMenu->addAction("Redenumire");
    QAction* deleteAction = editMenu->addAction("Sterge");
    editMenu->addSeparator();
    QAction* shareMenuAction  = editMenu->addAction("Partajare...");
    QAction* manageGroupsAction = editMenu->addAction("Gestionare Grupuri");
    QAction* propertiesAction = editMenu->addAction("Proprietati");
    renameAction->setShortcut(QKeySequence(Qt::Key_F2));
    propertiesAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Return));

    QMenu* viewMenu         = menuBar()->addMenu("View");
    QAction* sortNameAction = viewMenu->addAction("Sortare dupa nume");
    QAction* sortSizeAction = viewMenu->addAction("Sortare dupa dimensiune");
    QAction* sortDateAction = viewMenu->addAction("Sortare dupa data");

    QMenu* logMenu            = menuBar()->addMenu("Log");
    QAction* showLogsAction   = logMenu->addAction("Afiseaza log-uri");
    QAction* exportLogsAction = logMenu->addAction("Export log-uri");

    QMenu* helpMenu      = menuBar()->addMenu("Help");
    QAction* aboutAction = helpMenu->addAction("Despre ATMosFS");

    connect(exitAction,       &QAction::triggered, qApp, &QApplication::quit);
    connect(newFileAction,    &QAction::triggered, this, &MainWindow::onNewFile);
    connect(newFolderAction,  &QAction::triggered, this, &MainWindow::onNewFolder);
    connect(renameAction,     &QAction::triggered, this, &MainWindow::onRename);
    connect(deleteAction,     &QAction::triggered, this, &MainWindow::onDelete);
    connect(shareMenuAction,  &QAction::triggered, this, &MainWindow::onShareDialog);
    connect(manageGroupsAction, &QAction::triggered, this, &MainWindow::onManageGroups);
    connect(propertiesAction, &QAction::triggered, this, &MainWindow::onProperties);
    connect(sortNameAction,   &QAction::triggered, this, &MainWindow::onSortByName);
    connect(sortSizeAction,   &QAction::triggered, this, &MainWindow::onSortBySize);
    connect(sortDateAction,   &QAction::triggered, this, &MainWindow::onSortByDate);
    connect(showLogsAction,   &QAction::triggered, this, &MainWindow::onShowLogs);
    connect(exportLogsAction, &QAction::triggered, this, &MainWindow::onExportLogs);
    connect(aboutAction,      &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "Despre ATMosFS", "ATMosFS — Sistem de Fisiere Virtual\nProiect POO C++\n\nArhitectura: client-server, SQLite, Qt\nIndex: B+ Tree");
    });
}

void MainWindow::setupUI() {
    auto* centralWidget = new QWidget(this);
    auto* mainLayout    = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* topBar    = new QWidget(this);
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(8, 6, 8, 6);

    m_addressBar = new QLineEdit(this);
    m_addressBar->setPlaceholderText("Cale absoluta (ex: /root/Documents)");
    m_addressBar->setMinimumHeight(30);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("Cauta...");
    m_searchBar->setMinimumHeight(30);
    m_searchBar->setMaximumWidth(250);

    auto* searchBtn = new QPushButton("🔍", this);
    searchBtn->setFixedSize(32, 30);

    topLayout->addWidget(new QLabel("📁", this));
    topLayout->addWidget(m_addressBar, 1);
    topLayout->addWidget(m_searchBar);
    topLayout->addWidget(searchBtn);

    m_model    = new FileSystemModel(m_root, this);
    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setAnimated(true);
    m_treeView->setSortingEnabled(false);

    // ── FIX SORTARE LA CLICK ────────────────────────────────
    m_treeView->header()->setSectionsClickable(true);
    m_treeView->header()->setSortIndicatorShown(true);

    connect(m_treeView->header(), &QHeaderView::sortIndicatorChanged, this, [this](int logicalIndex, Qt::SortOrder order) {
        if (logicalIndex == 0) {
            m_currentSort = (order == Qt::AscendingOrder) ? SortManager::SortCrit::NAME_ASC : SortManager::SortCrit::NAME_DESC;
            m_root->sortChildrenRecursive(m_currentSort);
            m_model->refresh();
            m_statusLabel->setText(order == Qt::AscendingOrder ? "Sortat dupa: Nume A→Z" : "Sortat dupa: Nume Z→A");
        } else if (logicalIndex == 2) {
            m_currentSort = (order == Qt::AscendingOrder) ? SortManager::SortCrit::SIZE_ASC : SortManager::SortCrit::SIZE_DESC;
            m_root->sortChildrenRecursive(m_currentSort);
            m_model->refresh();
            m_statusLabel->setText(order == Qt::AscendingOrder ? "Sortat dupa: Dimensiune (mic→mare)" : "Sortat dupa: Dimensiune (mare→mic)");
        } else if (logicalIndex == 3) {
            m_currentSort = (order == Qt::AscendingOrder) ? SortManager::SortCrit::DATE_ASC : SortManager::SortCrit::DATE_DESC;
            m_root->sortChildrenRecursive(m_currentSort);
            m_model->refresh();
            m_statusLabel->setText(order == Qt::AscendingOrder ? "Sortat dupa: Data (vechi→nou)" : "Sortat dupa: Data (nou→vechi)");
        }
    });
    // ────────────────────────────────────────────────────────

    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeView->header()->resizeSection(1, 80);
    m_treeView->header()->resizeSection(2, 100);
    m_treeView->header()->resizeSection(3, 140);
    m_treeView->setMinimumWidth(400);

    auto* rightPanel  = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(8, 8, 8, 8);

    QFont boldFont;
    boldFont.setBold(true);

    auto* previewLabel = new QLabel("Preview", this);
    previewLabel->setFont(boldFont);

    m_previewPane = new QTextEdit(this);
    m_previewPane->setReadOnly(true);
    m_previewPane->setPlaceholderText("Selecteaza un element...");

    auto* resultsLabel = new QLabel("Rezultate cautare", this);
    resultsLabel->setFont(boldFont);

    m_searchResults = new QListWidget(this);
    m_searchResults->setMaximumHeight(180);

    auto* logLabel = new QLabel("Log-uri", this);
    logLabel->setFont(boldFont);

    m_logConsole = new QTextEdit(this);
    m_logConsole->setReadOnly(true);
    m_logConsole->setMaximumHeight(120);
    m_logConsole->setStyleSheet("background-color: #1E1E1E; color: #00FF00; font-family: Consolas;");

    rightLayout->addWidget(previewLabel);
    rightLayout->addWidget(m_previewPane, 1);
    rightLayout->addWidget(resultsLabel);
    rightLayout->addWidget(m_searchResults);
    rightLayout->addWidget(logLabel);
    rightLayout->addWidget(m_logConsole);

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->addWidget(m_treeView);
    m_mainSplitter->addWidget(rightPanel);
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 2);

    m_statusLabel = new QLabel("0 elemente | 0 B", this);
    m_statusLabel->setStyleSheet("padding: 4px 8px; background-color: #F0F0F0; border-top: 1px solid #CCCCCC;");

    mainLayout->addWidget(topBar);
    mainLayout->addWidget(m_mainSplitter, 1);
    mainLayout->addWidget(m_statusLabel);

    setCentralWidget(centralWidget);

    connect(m_treeView, &QTreeView::customContextMenuRequested, this, &MainWindow::onContextMenu);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::onItemSelected);
    connect(searchBtn, &QPushButton::clicked, this, &MainWindow::onSearchTriggered);
    connect(m_searchBar, &QLineEdit::returnPressed, this, &MainWindow::onSearchTriggered);
    connect(m_addressBar, &QLineEdit::returnPressed, this, &MainWindow::onAddressBarEntered);
    connect(m_searchResults, &QListWidget::itemClicked, this, &MainWindow::onSearchResultClicked);
}

// ── Permisiuni ───────────────────────────────────────

bool MainWindow::checkWritePermission(FileSystemEntity* entity) {
    if (!entity) return false;
    if (entity->getOwnerUser() == m_currentUser) return true;

    if (!m_fm.checkPermission(entity->getId(), m_currentUser, "write")) {
        QMessageBox::warning(this, "Acces refuzat",
                             QString("Nu aveti permisiunea de scriere asupra '%1'!\nContactati proprietarul pentru acces.")
                                 .arg(QString::fromStdString(entity->getName())));
        return false;
    }
    return true;
}

bool MainWindow::checkReadPermission(FileSystemEntity* entity) {
    if (!entity) return false;
    if (entity->getOwnerUser() == m_currentUser) return true;

    if (!m_fm.checkPermission(entity->getId(), m_currentUser, "read")) {
        QMessageBox::warning(this, "Acces refuzat",
                             QString("Nu aveti permisiunea de citire asupra '%1'!")
                                 .arg(QString::fromStdString(entity->getName())));
        return false;
    }
    return true;
}

// ── Navigare ─────────────────────────────────────────

void MainWindow::onItemSelected(const QModelIndex& index) {
    if (!index.isValid()) return;
    auto* entity = m_model->entityFromIndex(index);
    if (!entity) return;

    if (!checkReadPermission(entity)) return;

    showPreview(entity);
    updateStatusBar(entity);

    if (entity->isFolder()) {
        auto* folder = dynamic_cast<Folder*>(entity);
        if (folder) {
            m_addressBar->setText(QString::fromStdString(folder->getAbsolutePath()));
        }
    }
}

void MainWindow::onAddressBarEntered() {
    QString path = m_addressBar->text().trimmed();
    if (path.isEmpty()) return;

    if (!m_pathResolver->validatePath(path.toStdString())) {
        m_addressBar->setStyleSheet("border: 1px solid red;");
        m_addressBar->setText(QString::fromStdString(m_pathResolver->getLastValidPath()));
        QMessageBox::warning(this, "Cale inexistenta", "Calea introdusa nu exista in sistemul de fisiere!");
        return;
    }

    m_addressBar->setStyleSheet("");
    logEvent("NAVIGATE", path.toStdString());
}

// ── Cautare ──────────────────────────────────────────

void MainWindow::onSearchTriggered() {
    QString query = m_searchBar->text().trimmed();
    if (query.isEmpty()) return;

    m_searchResults->clear();
    auto results = m_searchEngine.searchByName(m_root, query.toStdString());

    if (results.empty()) {
        m_searchResults->addItem("Niciun rezultat pentru: " + query);
        m_statusLabel->setText("Cautare: niciun rezultat");
        return;
    }

    for (const auto& r : results) {
        QString icon = r.entity->isFolder() ? "📁" : "📄";
        QString name = QString::fromStdString(r.entity->getName());
        QString path = QString::fromStdString(r.absolutePath);

        auto* item = new QListWidgetItem(icon + " " + name + "  —  " + path);
        item->setData(Qt::UserRole, path);
        m_searchResults->addItem(item);
    }

    m_statusLabel->setText(QString("Cautare: '%1' — %2 rezultate").arg(query).arg(results.size()));
    logEvent("SEARCH", query.toStdString());
}

void MainWindow::onSearchResultClicked(QListWidgetItem* item) {
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    m_addressBar->setText(path);
}

// ── Creare fisier ────────────────────────────────────

void MainWindow::onNewFile() {
    auto index = m_treeView->currentIndex();
    Folder* parentFolder = m_root;
    int parentId = m_root->getId();

    if (index.isValid()) {
        auto* entity = m_model->entityFromIndex(index);
        if (entity && entity->isFolder()) {
            if (!checkWritePermission(entity)) return;
            parentFolder = dynamic_cast<Folder*>(entity);
            if (parentFolder) parentId = parentFolder->getId();
        } else if (entity && !entity->isFolder()) {
            auto parentIndex = m_model->parent(index);
            if (parentIndex.isValid()) {
                auto* parent = m_model->entityFromIndex(parentIndex);
                if (!checkWritePermission(parent)) return;
                parentFolder = dynamic_cast<Folder*>(parent);
                if (parentFolder) parentId = parentFolder->getId();
            }
        }
    }

    bool ok;
    QString name = QInputDialog::getText(this, "Fisier nou", "Numele fisierului (fara extensie):",
                                         QLineEdit::Normal, QString("Document Nou %1").arg(parentFolder->getChildCount() + 1), &ok);

    if (!ok || name.isEmpty()) return;

    try {
        bool saved = m_fm.createTextFile(name.toStdString(), m_currentUser, "users", "", parentId);
        if (!saved) { QMessageBox::warning(this, "Eroare", "Nu s-a putut salva fisierul in baza de date!"); return; }

        auto newFile = std::make_shared<TextFile>(name.toStdString(), m_currentUser, "users", "");
        parentFolder->addChild(newFile);

        m_model->refresh();
        if (index.isValid() && m_model->entityFromIndex(index) && m_model->entityFromIndex(index)->isFolder()) { m_treeView->expand(index); }

        logEvent("CREATE_FILE", name.toStdString());
        m_statusLabel->setText("Fisier creat: " + name);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

// ── Creare folder ────────────────────────────────────

void MainWindow::onNewFolder() {
    auto index = m_treeView->currentIndex();
    Folder* parentFolder = m_root;
    int parentId = m_root->getId();

    if (index.isValid()) {
        auto* entity = m_model->entityFromIndex(index);
        if (entity && entity->isFolder()) {
            if (!checkWritePermission(entity)) return;
            parentFolder = dynamic_cast<Folder*>(entity);
            if (parentFolder) parentId = parentFolder->getId();
        } else if (entity && !entity->isFolder()) {
            auto parentIndex = m_model->parent(index);
            if (parentIndex.isValid()) {
                auto* parent = m_model->entityFromIndex(parentIndex);
                if (!checkWritePermission(parent)) return;
                parentFolder = dynamic_cast<Folder*>(parent);
                if (parentFolder) parentId = parentFolder->getId();
            }
        }
    }

    bool ok;
    QString name = QInputDialog::getText(this, "Folder nou", "Numele folderului:",
                                         QLineEdit::Normal, QString("Folder Nou %1").arg(parentFolder->getChildCount() + 1), &ok);

    if (!ok || name.isEmpty()) return;

    try {
        bool saved = m_fm.createFolder(name.toStdString(), m_currentUser, "users", parentId);
        if (!saved) { QMessageBox::warning(this, "Eroare", "Nu s-a putut salva folderul in baza de date!"); return; }

        auto newFolder = std::make_shared<Folder>(name.toStdString(), m_currentUser, "users", parentFolder);
        parentFolder->addChild(newFolder);

        m_model->refresh();
        if (index.isValid() && m_model->entityFromIndex(index) && m_model->entityFromIndex(index)->isFolder()) { m_treeView->expand(index); }

        logEvent("CREATE_FOLDER", name.toStdString());
        m_statusLabel->setText("Folder creat: " + name);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

// ── Redenumire ───────────────────────────────────────

void MainWindow::onRename() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;

    auto* entity = m_model->entityFromIndex(index);
    if (!entity) return;

    if (!checkWritePermission(entity)) return;

    QString currentName = QString::fromStdString(entity->getName());

    bool ok;
    QString newName = QInputDialog::getText(this, "Redenumire", "Introdu noul nume:", QLineEdit::Normal, currentName, &ok);

    if (ok && !newName.isEmpty() && newName != currentName) {
        try {
            if (m_fm.renameEntity(entity->getId(), newName.toStdString(), m_currentUser)) {
                entity->setName(newName.toStdString());
                m_model->refresh();
                logEvent("RENAME", currentName.toStdString() + " -> " + newName.toStdString());
                m_statusLabel->setText("Redenumit in: " + newName);
            } else {
                QMessageBox::warning(this, "Eroare", "Eroare la actualizarea in baza de date!");
            }
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
        }
    }
}

// ── Stergere ─────────────────────────────────────────

void MainWindow::onDelete() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;

    auto* entity = m_model->entityFromIndex(index);
    if (!entity) return;

    if (!checkWritePermission(entity)) return;

    auto reply = QMessageBox::question(this, "Confirmare stergere",
                                       QString("Esti sigur ca vrei sa stergi '%1'?").arg(QString::fromStdString(entity->getName())),
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    try {
        int entityId = entity->getId();
        auto parentIndex = m_model->parent(index);
        Folder* parentFolder = m_root;

        if (parentIndex.isValid()) {
            auto* parent = m_model->entityFromIndex(parentIndex);
            parentFolder = dynamic_cast<Folder*>(parent);
        }

        if (parentFolder) {
            std::string name = entity->getName();
            m_fm.deleteEntity(entityId, m_currentUser);
            parentFolder->removeChild(name);
            m_model->refresh();
            logEvent("DELETE", name);
            m_statusLabel->setText(QString("Sters: %1").arg(QString::fromStdString(name)));
        }
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

// ── Deschide extern ──────────────────────────────────

void MainWindow::onOpenExternal() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;

    auto* entity = m_model->entityFromIndex(index);
    if (!entity || entity->isFolder()) return;

    if (!checkReadPermission(entity)) return;

    try {
        QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + QString::fromStdString(entity->getName());

        QFile realFile(tempPath);
        if (realFile.open(QIODevice::WriteOnly)) {
            auto* tf = dynamic_cast<TextFile*>(entity);
            if (tf) realFile.write(QString::fromStdString(tf->read()).toUtf8());
            realFile.close();
            QDesktopServices::openUrl(QUrl::fromLocalFile(tempPath));
            logEvent("OPEN_EXTERNAL", entity->getName());
            m_statusLabel->setText("Fisier deschis extern: " + QString::fromStdString(entity->getName()));
        } else {
            QMessageBox::warning(this, "Eroare", "Nu s-a putut crea fisierul temporar!");
        }
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

// ── Dialog gestionare grupuri ────────────────────────

void MainWindow::onManageGroups() {
    QDialog dialog(this);
    dialog.setWindowTitle("Gestionare Grupuri");
    dialog.setFixedSize(400, 450);

    auto* layout = new QVBoxLayout(&dialog);

    auto* groupNameInput = new QLineEdit(&dialog);
    groupNameInput->setPlaceholderText("Nume grup nou...");

    auto* createGroupBtn = new QPushButton("Creare Grup", &dialog);

    auto* addLayout = new QHBoxLayout();
    addLayout->addWidget(groupNameInput);
    addLayout->addWidget(createGroupBtn);
    layout->addLayout(addLayout);

    layout->addWidget(new QLabel("Membrii Grupului (selecteaza grupul pentru a vedea utilizatorii):", &dialog));

    auto* groupCombo = new QComboBox(&dialog);
    auto groups = m_auth.getAllGroups();
    for (const auto& g : groups) {
        groupCombo->addItem(QString::fromStdString(g));
    }
    layout->addWidget(groupCombo);

    auto* userList = new QListWidget(&dialog);
    auto allUsers = m_auth.getAllUsers();
    for (const auto& u : allUsers) {
        auto* item = new QListWidgetItem(QString::fromStdString(u.second), userList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    layout->addWidget(userList);

    auto* applyBtn = new QPushButton("Aplica modificarile pe grup", &dialog);
    layout->addWidget(applyBtn);

    auto refreshUsers = [&]() {
        if (groupCombo->currentIndex() < 0) return;
        std::string currentGroup = groupCombo->currentText().toStdString();
        auto members = m_auth.getUsersInGroup(currentGroup);
        for (int i = 0; i < userList->count(); ++i) {
            auto* item = userList->item(i);
            std::string uname = item->text().toStdString();
            bool isMember = std::find(members.begin(), members.end(), uname) != members.end();
            item->setCheckState(isMember ? Qt::Checked : Qt::Unchecked);
        }
    };

    connect(groupCombo, &QComboBox::currentIndexChanged, &dialog, refreshUsers);
    refreshUsers();

    connect(createGroupBtn, &QPushButton::clicked, &dialog, [&]() {
        std::string gName = groupNameInput->text().trimmed().toStdString();
        if (gName.empty()) return;

        if (m_auth.createGroup(gName)) {
            groupCombo->addItem(QString::fromStdString(gName));
            groupCombo->setCurrentIndex(groupCombo->count() - 1);
            groupNameInput->clear();
            QMessageBox::information(&dialog, "Succes", "Grupul a fost creat cu succes!");
        } else {
            QMessageBox::warning(&dialog, "Eroare", "Grupul exista deja sau a aparut o eroare!");
        }
    });

    connect(applyBtn, &QPushButton::clicked, &dialog, [&]() {
        if (groupCombo->currentIndex() < 0) return;
        std::string currentGroup = groupCombo->currentText().toStdString();

        int processed = 0;
        for (int i = 0; i < userList->count(); ++i) {
            auto* item = userList->item(i);
            std::string uname = item->text().toStdString();
            if (item->checkState() == Qt::Checked) {
                if (m_auth.addUserToGroup(currentGroup, uname)) processed++;
            } else {
                if (m_auth.removeUserFromGroup(currentGroup, uname)) processed++;
            }
        }
        QMessageBox::information(&dialog, "Succes", QString("Grupul '%1' a fost actualizat cu succes!").arg(QString::fromStdString(currentGroup)));
        refreshUsers();
    });

    dialog.exec();
}

// ── Dialog partajare ─────────────────────────────────

void MainWindow::showShareDialog(FileSystemEntity* entity) {
    if (!entity) return;

    if (entity->getOwnerUser() != m_currentUser) {
        QMessageBox::warning(this, "Acces refuzat", "Doar proprietarul poate partaja aceasta resursa!");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QString("Partajare — %1").arg(QString::fromStdString(entity->getName())));
    dialog.setFixedSize(400, 460);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* titleLabel = new QLabel(QString("<b>Partajeaza '%1' cu:</b>").arg(QString::fromStdString(entity->getName())), &dialog);
    layout->addWidget(titleLabel);

    auto* listWidget = new QListWidget(&dialog);
    listWidget->setSelectionMode(QAbstractItemView::NoSelection);

    auto allUsers = m_auth.getAllUsers();

    std::vector<std::string> alreadyShared = m_fm.getSharedWithList(entity->getId());

    for (const auto& [id, uname] : allUsers) {
        if (uname == m_currentUser) continue;

        auto* item = new QListWidgetItem(QString::fromStdString(uname), listWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

        bool isShared = std::find(alreadyShared.begin(), alreadyShared.end(), uname) != alreadyShared.end();
        item->setCheckState(isShared ? Qt::Checked : Qt::Unchecked);

        if (isShared) item->setForeground(QColor(0, 120, 215));
    }

    if (listWidget->count() == 0) listWidget->addItem("Nu exista alti utilizatori inregistrati.");
    layout->addWidget(listWidget);

    auto* permGroup  = new QGroupBox("Nivel acces acordat", &dialog);
    auto* permLayout = new QVBoxLayout(permGroup);
    auto* canReadCheck  = new QCheckBox("Poate Citi",     permGroup);
    auto* canWriteCheck = new QCheckBox("Poate Modifica", permGroup);
    canReadCheck->setChecked(true);
    permLayout->addWidget(canReadCheck);
    permLayout->addWidget(canWriteCheck);
    layout->addWidget(permGroup);

    auto* btnLayout = new QHBoxLayout();
    auto* cancelBtn = new QPushButton("Anuleaza", &dialog);
    auto* applyBtn  = new QPushButton("Aplica",   &dialog);
    applyBtn->setStyleSheet("QPushButton { background-color: #0078D4; color: white; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #006CBE; }");
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(applyBtn);
    layout->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    connect(applyBtn, &QPushButton::clicked, &dialog, [&]() {
        int sharedCount = 0;
        for (int i = 0; i < listWidget->count(); i++) {
            auto* item = listWidget->item(i);
            if (!item) continue;

            std::string uname = item->text().toStdString();
            bool isChecked = (item->checkState() == Qt::Checked);
            bool wasShared = std::find(alreadyShared.begin(), alreadyShared.end(), uname) != alreadyShared.end();

            if (isChecked && !wasShared) {
                try {
                    m_fm.shareEntity(entity->getId(), uname);
                    m_fm.addUserToGroup(entity->getId(), uname);
                    m_fm.setPermission(entity->getId(), uname, canReadCheck->isChecked(), canWriteCheck->isChecked());
                    sharedCount++;
                } catch (...) {}
            } else if (!isChecked && wasShared) {
                try {
                    m_fm.revokeShare(entity->getId(), uname);
                    m_fm.removeUserFromGroup(entity->getId(), uname);
                } catch (...) {}
            }
        }
        logEvent("SHARE", entity->getName() + " -> " + std::to_string(sharedCount) + " useri noi");
        dialog.accept();
    });

    if (dialog.exec() == QDialog::Accepted) {
        m_model->refresh();
        QMessageBox::information(this, "Partajare", "Setarile de partajare au fost aplicate si salvate!");
    }
}

// ── Dialog proprietati ───────────────────────────────

void MainWindow::showPropertiesDialog(FileSystemEntity* entity) {
    if (!entity) return;

    QDialog dialog(this);
    dialog.setWindowTitle(QString("Proprietati — %1").arg(QString::fromStdString(entity->getName())));
    dialog.setFixedSize(360, 480);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* infoGroup  = new QGroupBox("Informatii", &dialog);
    auto* infoLayout = new QFormLayout(infoGroup);

    infoLayout->addRow("Nume:", new QLabel(QString::fromStdString(entity->getName())));
    infoLayout->addRow("Tip:", new QLabel(entity->isFolder() ? "Director" : "Fisier"));
    infoLayout->addRow("Proprietar:", new QLabel(QString::fromStdString(entity->getOwnerUser())));
    infoLayout->addRow("Grup:", new QLabel(QString::fromStdString(entity->getOwnerGroup())));
    infoLayout->addRow("Dimensiune:", new QLabel(QString::number(entity->getSize()) + " bytes"));

    std::time_t t = entity->getCreatedAt();
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    infoLayout->addRow("Creat la:", new QLabel(QString(buf)));

    layout->addWidget(infoGroup);

    bool isOwner = (entity->getOwnerUser() == m_currentUser);

    auto* permGroup  = new QGroupBox("Permisiuni", &dialog);
    auto* permLayout = new QVBoxLayout(permGroup);

    permLayout->addWidget(new QLabel("<b>Owner:</b> Read + Write (implicit)"));
    permLayout->addSpacing(6);

    permLayout->addWidget(new QLabel("<b>Grup:</b>"));
    auto* groupReadCheck  = new QCheckBox("Poate Citi",     permGroup);
    auto* groupWriteCheck = new QCheckBox("Poate Modifica", permGroup);
    permLayout->addWidget(groupReadCheck);
    permLayout->addWidget(groupWriteCheck);
    permLayout->addSpacing(6);

    permLayout->addWidget(new QLabel("<b>Others:</b>"));
    auto* othersReadCheck  = new QCheckBox("Poate Citi",     permGroup);
    auto* othersWriteCheck = new QCheckBox("Poate Modifica", permGroup);
    permLayout->addWidget(othersReadCheck);
    permLayout->addWidget(othersWriteCheck);

    try {
        auto pm = m_fm.loadPermissions(entity->getId());

        auto gp = pm.getGroupPermission();
        if (gp) {
            groupReadCheck->setChecked(gp->canRead());
            groupWriteCheck->setChecked(gp->canWrite());
        }

        auto op = pm.getOthersPermission();
        if (op) {
            othersReadCheck->setChecked(op->canRead());
            othersWriteCheck->setChecked(op->canWrite());
        }
    } catch (...) {}

    if (!isOwner) {
        groupReadCheck->setEnabled(false);
        groupWriteCheck->setEnabled(false);
        othersReadCheck->setEnabled(false);
        othersWriteCheck->setEnabled(false);
    }

    layout->addWidget(permGroup);

    auto* btnLayout = new QHBoxLayout();
    auto* closeBtn  = new QPushButton("Inchide", &dialog);
    auto* applyBtn  = new QPushButton("Aplica",  &dialog);
    applyBtn->setEnabled(isOwner);
    applyBtn->setStyleSheet("QPushButton { background-color: #0078D4; color: white; border-radius: 4px; font-weight: bold; } QPushButton:hover { background-color: #006CBE; } QPushButton:disabled { background-color: #CCCCCC; color: #888888; }");

    btnLayout->addWidget(closeBtn);
    btnLayout->addWidget(applyBtn);
    layout->addLayout(btnLayout);

    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    connect(applyBtn, &QPushButton::clicked, &dialog, [&]() {
        try {
            m_fm.updateGroupPermissions(entity->getId(), groupReadCheck->isChecked(), groupWriteCheck->isChecked());
            m_fm.updateOthersPermissions(entity->getId(), othersReadCheck->isChecked(), othersWriteCheck->isChecked());

            logEvent("PERMISSIONS", entity->getName() +
                                        " group[r=" + (groupReadCheck->isChecked()  ? "1" : "0") + " w=" + (groupWriteCheck->isChecked() ? "1" : "0") +
                                        "] others[r=" + (othersReadCheck->isChecked()  ? "1" : "0") + " w=" + (othersWriteCheck->isChecked() ? "1" : "0") + "]");

            dialog.accept();
            QMessageBox::information(this, "Permisiuni", "Permisiunile au fost actualizate si salvate!");
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
        }
    });

    dialog.exec();
}

// ── Context menu ─────────────────────────────────────

void MainWindow::onContextMenu(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);
    QMenu contextMenu(this);

    QAction* newFileAction   = contextMenu.addAction("📄 Fisier nou");
    QAction* newFolderAction = contextMenu.addAction("📁 Folder nou");
    contextMenu.addSeparator();

    QAction* openExternalAction = nullptr;
    QAction* renameAction       = nullptr;
    QAction* deleteAction       = nullptr;
    QAction* shareAction        = nullptr;
    QAction* propertiesAction   = nullptr;

    FileSystemEntity* entity = nullptr;

    if (index.isValid()) {
        entity = m_model->entityFromIndex(index);
        if (entity) {
            if (!entity->isFolder()) {
                openExternalAction = contextMenu.addAction("🖥 Deschide extern");
                contextMenu.addSeparator();
            }

            renameAction = contextMenu.addAction("✏ Redenumire (F2)");

            deleteAction = contextMenu.addAction("🗑 Sterge");
            bool canWrite = (entity->getOwnerUser() == m_currentUser) || m_fm.checkPermission(entity->getId(), m_currentUser, "write");
            if (!canWrite) {
                deleteAction->setEnabled(false);
                deleteAction->setToolTip("Nu aveti permisiunea de scriere!");
            }

            contextMenu.addSeparator();

            shareAction = contextMenu.addAction("🔗 Partajare...");
            if (entity->getOwnerUser() != m_currentUser) {
                shareAction->setEnabled(false);
                shareAction->setToolTip("Doar proprietarul poate partaja!");
            }

            propertiesAction = contextMenu.addAction("⚙ Proprietati");
        }
    }

    QAction* selected = contextMenu.exec(m_treeView->viewport()->mapToGlobal(pos));
    if (!selected) return;

    if (selected == newFileAction)   { onNewFile();   return; }
    if (selected == newFolderAction) { onNewFolder(); return; }
    if (renameAction && selected == renameAction) { onRename(); return; }
    if (deleteAction && selected == deleteAction) { onDelete(); return; }
    if (openExternalAction && selected == openExternalAction) { onOpenExternal(); return; }
    if (shareAction && selected == shareAction) { showShareDialog(entity); return; }
    if (propertiesAction && selected == propertiesAction) { showPropertiesDialog(entity); return; }
}

// ── Sloturi dialog ───────────────────────────────────

void MainWindow::onProperties() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    auto* entity = m_model->entityFromIndex(index);
    if (entity) showPropertiesDialog(entity);
}

void MainWindow::onShareDialog() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    auto* entity = m_model->entityFromIndex(index);
    if (entity) showShareDialog(entity);
}

// ── FIX: Trigger UI ca sageata sa faca toggle la actiunile din Meniu

void MainWindow::onSortByName() {
    if (m_treeView->header()->sortIndicatorSection() == 0) {
        Qt::SortOrder next = (m_treeView->header()->sortIndicatorOrder() == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        m_treeView->header()->setSortIndicator(0, next);
    } else {
        m_treeView->header()->setSortIndicator(0, Qt::AscendingOrder);
    }
}

void MainWindow::onSortBySize() {
    if (m_treeView->header()->sortIndicatorSection() == 2) {
        Qt::SortOrder next = (m_treeView->header()->sortIndicatorOrder() == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        m_treeView->header()->setSortIndicator(2, next);
    } else {
        m_treeView->header()->setSortIndicator(2, Qt::AscendingOrder);
    }
}

void MainWindow::onSortByDate() {
    if (m_treeView->header()->sortIndicatorSection() == 3) {
        Qt::SortOrder next = (m_treeView->header()->sortIndicatorOrder() == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        m_treeView->header()->setSortIndicator(3, next);
    } else {
        m_treeView->header()->setSortIndicator(3, Qt::AscendingOrder);
    }
}

// ── Logger ───────────────────────────────────────────

void MainWindow::onShowLogs() {
    if (m_logConsole) m_logConsole->show();
}

void MainWindow::onExportLogs() {
    if (m_logger) {
        m_logger->exportLogs("export_logs.txt");
        QMessageBox::information(this, "Export", "Log-urile au fost exportate in 'export_logs.txt'!");
    }
}

// ── Helper ───────────────────────────────────────────

void MainWindow::showPreview(FileSystemEntity* entity) {
    if (!entity) return;
    m_previewPane->clear();

    QString info;
    info += "<b>Nume:</b> " + QString::fromStdString(entity->getName()) + "<br>";
    info += "<b>Proprietar:</b> " + QString::fromStdString(entity->getOwnerUser()) + "<br>";
    info += "<b>Grup:</b> " + QString::fromStdString(entity->getOwnerGroup()) + "<br>";
    info += "<b>Dimensiune:</b> " + QString::number(entity->getSize()) + " bytes<br>";

    std::time_t t = entity->getCreatedAt();
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    info += "<b>Creat la:</b> " + QString(buf) + "<br>";
    info += "<hr>";

    if (entity->isFolder()) {
        auto* folder = dynamic_cast<Folder*>(entity);
        if (folder) {
            info += "<b>Tip:</b> Director<br>";
            info += "<b>Elemente:</b> " + QString::number(folder->getChildCount()) + "<br>";
        }

        auto shared = m_fm.getSharedWithList(entity->getId());
        if (!shared.empty()) {
            info += "<b>Partajat cu:</b> ";
            for (const auto& u : shared) info += QString::fromStdString(u) + " ";
            info += "<br>";
        }
    } else {
        auto* tf = dynamic_cast<TextFile*>(entity);
        if (tf) {
            info += "<b>Tip:</b> Fisier text<br><hr>";
            info += "<pre>" + QString::fromStdString(tf->read()) + "</pre>";

            auto shared = m_fm.getSharedWithList(entity->getId());
            if (!shared.empty()) {
                info += "<b>Partajat cu:</b> ";
                for (const auto& u : shared) info += QString::fromStdString(u) + " ";
                info += "<br>";
            }
        } else {
            info += "<b>Tip:</b> Fisier binar<br>";
            info += "<i>Continutul nu poate fi previzualizat.</i>";
        }
    }

    m_previewPane->setHtml(info);
}

void MainWindow::updateStatusBar(FileSystemEntity* entity) {
    if (!entity) return;

    if (entity->isFolder()) {
        auto* folder = dynamic_cast<Folder*>(entity);
        if (folder) {
            m_statusLabel->setText(QString("%1 elemente | %2 B").arg(folder->getChildCount()).arg(folder->getSize()));
        }
    } else {
        m_statusLabel->setText(QString("1 element | %1 B").arg(entity->getSize()));
    }
}

void MainWindow::logEvent(const std::string& action, const std::string& path) {
    if (m_logger) {
        std::string msg = "[" + m_currentUser + "] " + action;
        if (!path.empty()) msg += " -> " + path;
        m_logger->log(msg);

        if (m_logConsole) {
            m_logConsole->append(QString::fromStdString(msg));
        }
    }
}