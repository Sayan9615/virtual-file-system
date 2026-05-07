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
#include <fstream>

MainWindow::MainWindow(Folder* root,
                       const std::string& username,
                       FileManager& fm,
                       QWidget* parent)
    : QMainWindow(parent),
    m_root(root),
    m_currentUser(username),
    m_fm(fm),
    m_model(nullptr),
    m_searchEngine(),
    m_pathResolver(nullptr),
    m_logger(nullptr),
    m_currentSort(SortManager::SortCrit::NAME_ASC),
    m_treeView(nullptr),
    m_previewPane(nullptr),
    m_searchBar(nullptr),
    m_addressBar(nullptr),
    m_searchResults(nullptr),
    m_mainSplitter(nullptr),
    m_statusLabel(nullptr),
    m_logConsole(nullptr)
{
    m_pathResolver = new PathResolver(
        std::shared_ptr<Folder>(root, [](Folder*){}));
    m_logger = new TimestampedLogger("logs.txt");

    setWindowTitle(QString("ATMosFS — %1")
                       .arg(QString::fromStdString(username)));
    setMinimumSize(1024, 680);
    setupMenuBar();
    setupUI();
}

// ── Close / Save / Load ──────────────────────────────

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSystemState();
    QMainWindow::closeEvent(event);
}

void MainWindow::saveSystemState() {
    std::ofstream file("vfs_data.txt");
    if (file.is_open()) {
        file << m_root->serialize();
        file.close();
        logEvent("SYSTEM", "Datele au fost salvate pe disc.");
    }
}

void MainWindow::loadSystemState() {
    std::ifstream file("vfs_data.txt");
    if (file.is_open()) {
        std::string data((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
        if (!data.empty()) {
            m_root->deserialize(data);
        }
        file.close();
        logEvent("SYSTEM", "Datele au fost incarcate cu succes.");
        if (m_model) m_model->refresh();
    }
}

// ── Setup ────────────────────────────────────────────

void MainWindow::setupMenuBar() {
    QMenu* fileMenu      = menuBar()->addMenu("File");
    QAction* newFileAction   = fileMenu->addAction("Fisier nou");
    QAction* newFolderAction = fileMenu->addAction("Folder nou");
    fileMenu->addSeparator();
    QAction* exitAction  = fileMenu->addAction("Iesire");

    QMenu* editMenu      = menuBar()->addMenu("Edit");
    QAction* renameAction = editMenu->addAction("Redenumire");
    QAction* deleteAction = editMenu->addAction("Sterge");
    renameAction->setShortcut(QKeySequence(Qt::Key_F2));

    QMenu* viewMenu      = menuBar()->addMenu("View");
    QAction* sortNameAction = viewMenu->addAction("Sortare dupa nume");
    QAction* sortSizeAction = viewMenu->addAction("Sortare dupa dimensiune");
    QAction* sortDateAction = viewMenu->addAction("Sortare dupa data");

    QMenu* logMenu       = menuBar()->addMenu("Log");
    QAction* showLogsAction   = logMenu->addAction("Afiseaza log-uri");
    QAction* exportLogsAction = logMenu->addAction("Export log-uri");

    QMenu* helpMenu      = menuBar()->addMenu("Help");
    QAction* aboutAction = helpMenu->addAction("Despre ATMosFS");

    connect(exitAction,       &QAction::triggered, qApp, &QApplication::quit);
    connect(newFileAction,    &QAction::triggered, this, &MainWindow::onNewFile);
    connect(newFolderAction,  &QAction::triggered, this, &MainWindow::onNewFolder);
    connect(renameAction,     &QAction::triggered, this, &MainWindow::onRename);
    connect(deleteAction,     &QAction::triggered, this, &MainWindow::onDelete);
    connect(sortNameAction,   &QAction::triggered, this, &MainWindow::onSortByName);
    connect(sortSizeAction,   &QAction::triggered, this, &MainWindow::onSortBySize);
    connect(sortDateAction,   &QAction::triggered, this, &MainWindow::onSortByDate);
    connect(showLogsAction,   &QAction::triggered, this, &MainWindow::onShowLogs);
    connect(exportLogsAction, &QAction::triggered, this, &MainWindow::onExportLogs);
    connect(aboutAction,      &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "Despre ATMosFS",
                           "ATMosFS — Sistem de Fisiere Virtual\n"
                           "Proiect POO C++\n\n"
                           "Arhitectura: client-server, SQLite, Qt\n"
                           "Index: B+ Tree");
    });
}

void MainWindow::setupUI() {
    auto* centralWidget = new QWidget(this);
    auto* mainLayout    = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Top bar ──────────────────────────────────────
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

    // ── Tree View ────────────────────────────────────
    m_model    = new FileSystemModel(m_root, this);
    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setAnimated(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeView->header()->resizeSection(1, 80);
    m_treeView->header()->resizeSection(2, 100);
    m_treeView->header()->resizeSection(3, 140);
    m_treeView->setMinimumWidth(400);

    // ── Panel dreapta ────────────────────────────────
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
    m_logConsole->setStyleSheet(
        "background-color: #1E1E1E; color: #00FF00; font-family: Consolas;");

    rightLayout->addWidget(previewLabel);
    rightLayout->addWidget(m_previewPane, 1);
    rightLayout->addWidget(resultsLabel);
    rightLayout->addWidget(m_searchResults);
    rightLayout->addWidget(logLabel);
    rightLayout->addWidget(m_logConsole);

    // ── Splitter ─────────────────────────────────────
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->addWidget(m_treeView);
    m_mainSplitter->addWidget(rightPanel);
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 2);

    // ── Status bar ───────────────────────────────────
    m_statusLabel = new QLabel("0 elemente | 0 B", this);
    m_statusLabel->setStyleSheet(
        "padding: 4px 8px; background-color: #F0F0F0; "
        "border-top: 1px solid #CCCCCC;");

    mainLayout->addWidget(topBar);
    mainLayout->addWidget(m_mainSplitter, 1);
    mainLayout->addWidget(m_statusLabel);

    setCentralWidget(centralWidget);

    // ── Conexiuni ────────────────────────────────────
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::onContextMenu);

    connect(m_treeView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this, &MainWindow::onItemSelected);

    connect(searchBtn, &QPushButton::clicked,
            this, &MainWindow::onSearchTriggered);

    connect(m_searchBar, &QLineEdit::returnPressed,
            this, &MainWindow::onSearchTriggered);

    connect(m_addressBar, &QLineEdit::returnPressed,
            this, &MainWindow::onAddressBarEntered);

    connect(m_searchResults, &QListWidget::itemClicked,
            this, &MainWindow::onSearchResultClicked);
}

// ── Slots navigare ───────────────────────────────────

void MainWindow::onItemSelected(const QModelIndex& index) {
    if (!index.isValid()) return;

    auto* entity = m_model->entityFromIndex(index);
    if (!entity) return;

    showPreview(entity);
    updateStatusBar(entity);

    if (entity->isFolder()) {
        auto* folder = dynamic_cast<Folder*>(entity);
        if (folder) {
            m_addressBar->setText(
                QString::fromStdString(folder->getAbsolutePath()));
        }
    }
}

void MainWindow::onAddressBarEntered() {
    QString path = m_addressBar->text().trimmed();
    if (path.isEmpty()) return;

    if (!m_pathResolver->validatePath(path.toStdString())) {
        m_addressBar->setStyleSheet("border: 1px solid red;");
        m_addressBar->setText(
            QString::fromStdString(m_pathResolver->getLastValidPath()));
        QMessageBox::warning(this, "Cale inexistenta",
                             "Calea introdusa nu exista in sistemul de fisiere!");
        return;
    }

    m_addressBar->setStyleSheet("");
    logEvent("NAVIGATE", path.toStdString());
}

// ── Slots cautare ────────────────────────────────────

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

    m_statusLabel->setText(
        QString("Cautare: '%1' — %2 rezultate")
            .arg(query).arg(results.size()));

    logEvent("SEARCH", query.toStdString());
}

void MainWindow::onSearchResultClicked(QListWidgetItem* item) {
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    m_addressBar->setText(path);
}

// ── Slots operatii fisiere ───────────────────────────

void MainWindow::onNewFile() {
    auto index = m_treeView->currentIndex();
    Folder* parentFolder = m_root;
    int parentId = m_root->getId();

    if (index.isValid()) {
        auto* entity = m_model->entityFromIndex(index);
        if (entity && entity->isFolder()) {
            parentFolder = dynamic_cast<Folder*>(entity);
            if (parentFolder) parentId = parentFolder->getId();
        } else if (entity && !entity->isFolder()) {
            auto parentIndex = m_model->parent(index);
            if (parentIndex.isValid()) {
                auto* parent = m_model->entityFromIndex(parentIndex);
                parentFolder = dynamic_cast<Folder*>(parent);
                if (parentFolder) parentId = parentFolder->getId();
            }
        }
    }

    bool ok;
    QString name = QInputDialog::getText(this,
                                         "Fisier nou", "Numele fisierului (fara extensie):",
                                         QLineEdit::Normal,
                                         QString("Document Nou %1").arg(parentFolder->getChildCount() + 1),
                                         &ok);

    if (!ok || name.isEmpty()) return;

    try {
        // ── Salveaza in DB ────────────────────────────
        bool saved = m_fm.createTextFile(
            name.toStdString(),
            m_currentUser,
            "users",
            "",
            parentId);

        if (!saved) {
            QMessageBox::warning(this, "Eroare",
                                 "Nu s-a putut salva fisierul in baza de date!");
            return;
        }

        // ── Adauga in memorie ─────────────────────────
        auto newFile = std::make_shared<TextFile>(
            name.toStdString(), m_currentUser, "users", "");
        parentFolder->addChild(newFile);
        m_model->refresh();

        if (index.isValid() && m_model->entityFromIndex(index)->isFolder()) {
            m_treeView->expand(index);
        }

        logEvent("CREATE_FILE", name.toStdString());
        m_statusLabel->setText("Fisier creat: " + name);

    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

void MainWindow::onNewFolder() {
    auto index = m_treeView->currentIndex();
    Folder* parentFolder = m_root;
    int parentId = m_root->getId();

    if (index.isValid()) {
        auto* entity = m_model->entityFromIndex(index);
        if (entity && entity->isFolder()) {
            parentFolder = dynamic_cast<Folder*>(entity);
            if (parentFolder) parentId = parentFolder->getId();
        } else if (entity && !entity->isFolder()) {
            auto parentIndex = m_model->parent(index);
            if (parentIndex.isValid()) {
                auto* parent = m_model->entityFromIndex(parentIndex);
                parentFolder = dynamic_cast<Folder*>(parent);
                if (parentFolder) parentId = parentFolder->getId();
            }
        }
    }

    bool ok;
    QString name = QInputDialog::getText(this,
                                         "Folder nou", "Numele folderului:",
                                         QLineEdit::Normal,
                                         QString("Folder Nou %1").arg(parentFolder->getChildCount() + 1),
                                         &ok);

    if (!ok || name.isEmpty()) return;

    try {
        // ── Salveaza in DB ────────────────────────────
        bool saved = m_fm.createFolder(
            name.toStdString(),
            m_currentUser,
            "users",
            parentId);

        if (!saved) {
            QMessageBox::warning(this, "Eroare",
                                 "Nu s-a putut salva folderul in baza de date!");
            return;
        }

        // ── Adauga in memorie ─────────────────────────
        auto newFolder = std::make_shared<Folder>(
            name.toStdString(), m_currentUser, "users", parentFolder);
        parentFolder->addChild(newFolder);
        m_model->refresh();

        if (index.isValid() && m_model->entityFromIndex(index)->isFolder()) {
            m_treeView->expand(index);
        }

        logEvent("CREATE_FOLDER", name.toStdString());
        m_statusLabel->setText("Folder creat: " + name);

    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

void MainWindow::onRename() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;

    auto* entity = m_model->entityFromIndex(index);
    if (!entity) return;

    QString currentName = QString::fromStdString(entity->getName());

    bool ok;
    QString newName = QInputDialog::getText(this,
                                            "Redenumire", "Introdu noul nume:",
                                            QLineEdit::Normal, currentName, &ok);

    if (ok && !newName.isEmpty() && newName != currentName) {
        try {
            entity->setName(newName.toStdString());
            m_model->refresh();
            logEvent("RENAME",
                     currentName.toStdString() + " -> " + newName.toStdString());
            m_statusLabel->setText("Redenumit in: " + newName);
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
        }
    }
}

void MainWindow::onDelete() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;

    auto* entity = m_model->entityFromIndex(index);
    if (!entity) return;

    auto reply = QMessageBox::question(this,
                                       "Confirmare stergere",
                                       QString("Esti sigur ca vrei sa stergi '%1'?")
                                           .arg(QString::fromStdString(entity->getName())),
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

            // ── Sterge din DB ─────────────────────────
            m_fm.deleteEntity(entityId, m_currentUser);

            // ── Sterge din memorie ────────────────────
            parentFolder->removeChild(name);
            m_model->refresh();

            logEvent("DELETE", name);
            m_statusLabel->setText(
                QString("Sters: %1").arg(QString::fromStdString(name)));
        }

    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

void MainWindow::onOpenExternal() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;

    auto* entity = m_model->entityFromIndex(index);
    if (!entity || entity->isFolder()) return;

    try {
        QString tempPath =
            QStandardPaths::writableLocation(QStandardPaths::TempLocation)
            + "/" + QString::fromStdString(entity->getName());

        QFile realFile(tempPath);
        if (realFile.open(QIODevice::WriteOnly)) {
            auto* tf = dynamic_cast<TextFile*>(entity);
            if (tf) {
                realFile.write(QString::fromStdString(tf->read()).toUtf8());
            }
            realFile.close();

            QDesktopServices::openUrl(QUrl::fromLocalFile(tempPath));
            logEvent("OPEN_EXTERNAL", entity->getName());
            m_statusLabel->setText(
                "Fisier deschis extern: " +
                QString::fromStdString(entity->getName()));
        } else {
            QMessageBox::warning(this, "Eroare",
                                 "Nu s-a putut crea fisierul temporar!");
        }
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
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

    if (index.isValid()) {
        auto* entity = m_model->entityFromIndex(index);
        if (entity) {
            if (!entity->isFolder()) {
                openExternalAction = contextMenu.addAction("🖥 Deschide extern");
                contextMenu.addSeparator();
            }
            renameAction = contextMenu.addAction("✏ Redenumire (F2)");
            deleteAction = contextMenu.addAction("🗑 Sterge");
            contextMenu.addSeparator();
            shareAction  = contextMenu.addAction("🔗 Partajare...");
        }
    }

    QAction* selected = contextMenu.exec(
        m_treeView->viewport()->mapToGlobal(pos));

    if (!selected) return;

    if (selected == newFileAction)   { onNewFile();   return; }
    if (selected == newFolderAction) { onNewFolder(); return; }
    if (renameAction && selected == renameAction) { onRename(); return; }
    if (deleteAction && selected == deleteAction) { onDelete(); return; }
    if (openExternalAction && selected == openExternalAction) {
        onOpenExternal();
        return;
    }

    if (shareAction && selected == shareAction) {
        auto* entity = m_model->entityFromIndex(index);
        if (!entity) return;

        bool ok;
        QString user = QInputDialog::getText(this,
                                             "Partajare", "Username cu care partajezi:",
                                             QLineEdit::Normal, "", &ok);

        if (!ok || user.isEmpty()) return;

        // ── Validare username ─────────────────────────
        if (user.length() > 50) {
            QMessageBox::warning(this, "Eroare",
                                 "Username-ul este prea lung! (max 50 caractere)");
            return;
        }
        if (user.contains(' ')) {
            QMessageBox::warning(this, "Eroare",
                                 "Username-ul nu poate contine spatii!");
            return;
        }

        try {
            // Cazul 1 — e deja SharedFolder
            auto* sharedFolder = dynamic_cast<SharedFolder*>(entity);
            if (sharedFolder) {
                sharedFolder->share(user.toStdString());
                logEvent("SHARE",
                         entity->getName() + " -> " + user.toStdString());
                QMessageBox::information(this, "Partajare",
                                         QString("'%1' partajat si cu '%2'!")
                                             .arg(QString::fromStdString(entity->getName()), user));
                return;
            }

            // Cazul 2 — e Folder normal, upgrade la SharedFolder
            auto* folder = dynamic_cast<Folder*>(entity);
            if (folder) {
                Folder* parent = folder->getParent();
                if (!parent) {
                    QMessageBox::warning(this, "Eroare",
                                         "Folderul Root nu poate fi partajat!");
                    return;
                }

                auto newShared = std::make_shared<SharedFolder>(
                    folder->getName(),
                    folder->getOwnerUser(),
                    folder->getOwnerGroup(),
                    parent);

                for (auto& child : folder->getChildren()) {
                    if (auto* cf = dynamic_cast<Folder*>(child.get())) {
                        cf->setParent(newShared.get());
                    }
                    newShared->addChild(child);
                }

                newShared->share(user.toStdString());
                parent->removeChild(folder->getName());
                parent->addChild(newShared);
                m_model->refresh();

                logEvent("UPGRADE_AND_SHARE",
                         entity->getName() + " -> " + user.toStdString());
                QMessageBox::information(this, "Partajare reusita",
                                         QString("Folderul '%1' a devenit SharedFolder si a fost partajat cu '%2'!")
                                             .arg(QString::fromStdString(entity->getName()), user));
                return;
            }

            // Cazul 3 — e fisier (TextFile/BinaryFile)
            auto* shareable = dynamic_cast<iShareable*>(entity);
            if (shareable) {
                shareable->share(user.toStdString());
                logEvent("SHARE_FILE",
                         entity->getName() + " -> " + user.toStdString());
                QMessageBox::information(this, "Partajare",
                                         QString("Fisierul '%1' partajat cu '%2'!")
                                             .arg(QString::fromStdString(entity->getName()), user));
            }

        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Eroare",
                                 QString::fromStdString(e.what()));
        }
    }
}

// ── Sortare ──────────────────────────────────────────

void MainWindow::onSortByName() {
    m_currentSort = SortManager::SortCrit::NAME_ASC;
    m_statusLabel->setText("Sortat dupa: Nume");
    logEvent("SORT", "name");
}

void MainWindow::onSortBySize() {
    m_currentSort = SortManager::SortCrit::SIZE_DESC;
    m_statusLabel->setText("Sortat dupa: Dimensiune");
    logEvent("SORT", "size");
}

void MainWindow::onSortByDate() {
    m_currentSort = SortManager::SortCrit::DATE_DESC;
    m_statusLabel->setText("Sortat dupa: Data");
    logEvent("SORT", "date");
}

// ── Logger ───────────────────────────────────────────

void MainWindow::onShowLogs() {
    if (m_logConsole) m_logConsole->show();
}

void MainWindow::onExportLogs() {
    if (m_logger) {
        m_logger->exportLogs("export_logs.txt");
        QMessageBox::information(this, "Export",
                                 "Log-urile au fost exportate in 'export_logs.txt'!");
    }
}

// ── Helper ───────────────────────────────────────────

void MainWindow::showPreview(FileSystemEntity* entity) {
    if (!entity) return;
    m_previewPane->clear();

    QString info;
    info += "<b>Nume:</b> " +
            QString::fromStdString(entity->getName()) + "<br>";
    info += "<b>Proprietar:</b> " +
            QString::fromStdString(entity->getOwnerUser()) + "<br>";
    info += "<b>Dimensiune:</b> " +
            QString::number(entity->getSize()) + " bytes<br>";

    std::time_t t = entity->getCreatedAt();
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S",
                  std::localtime(&t));
    info += "<b>Creat la:</b> " + QString(buf) + "<br><hr>";

    if (entity->isFolder()) {
        auto* folder = dynamic_cast<Folder*>(entity);
        info += "<b>Tip:</b> Director<br>";
        info += "<b>Elemente:</b> " +
                QString::number(folder->getChildCount()) + "<br>";

        auto* sf = dynamic_cast<SharedFolder*>(entity);
        if (sf) {
            info += "<b>Partajat cu:</b> ";
            for (const auto& u : sf->getSharedWith()) {
                info += QString::fromStdString(u) + " ";
            }
            info += "<br>";
        }
    } else {
        auto* tf = dynamic_cast<TextFile*>(entity);
        if (tf) {
            info += "<b>Tip:</b> Fisier text<br><hr>";
            info += "<pre>" +
                    QString::fromStdString(tf->read()) + "</pre>";
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
        m_statusLabel->setText(
            QString("%1 elemente | %2 B")
                .arg(folder->getChildCount())
                .arg(folder->getSize()));
    } else {
        m_statusLabel->setText(
            QString("1 element | %1 B").arg(entity->getSize()));
    }
}

void MainWindow::logEvent(const std::string& action,
                          const std::string& path) {
    if (m_logger) {
        std::string msg = "[" + m_currentUser + "] " + action;
        if (!path.empty()) msg += " -> " + path;
        m_logger->log(msg);

        if (m_logConsole) {
            m_logConsole->append(QString::fromStdString(msg));
        }
    }
}