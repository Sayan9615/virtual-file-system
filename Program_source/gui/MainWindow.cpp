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
#include <fstream>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>
#include <QStandardPaths>



void MainWindow::onOpenExternal() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;

    auto* entity = m_model->entityFromIndex(index);
    if (!entity || entity->isFolder()) return;

    try {
        // Salvăm fișierul virtual într-un folder temporar real de pe Windows
        QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + QString::fromStdString(entity->getName());
        QFile realFile(tempPath);

        if (realFile.open(QIODevice::WriteOnly)) {
            // Dacă e fișier text, îi scriem conținutul (adaptează aici în funcție de metodele claselor tale TextFile/BinaryFile)
            auto* tf = dynamic_cast<TextFile*>(entity);
            if (tf) {
                realFile.write(QString::fromStdString(tf->read()).toUtf8());
            } else {
                // Dacă ai BinaryFile, îi scrii byte-array-ul aici
                // realFile.write( ... byte array din fisierul binar ... );
            }
            realFile.close();

            // Zicem Windows-ului: Deschide fișierul ăsta cu programul tău default!
            QDesktopServices::openUrl(QUrl::fromLocalFile(tempPath));

            logEvent("OPEN_EXTERNAL", entity->getName());
            m_statusLabel->setText("Fișier deschis în aplicație externă: " + QString::fromStdString(entity->getName()));
        } else {
            QMessageBox::warning(this, "Eroare", "Nu s-a putut crea fișierul temporar pentru deschidere.");
        }
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

void MainWindow::saveSystemState() {
    std::ofstream file("vfs_data.txt");
    if (file.is_open()) {
        // Apelăm metoda ta serialize pe folderul root
        file << m_root->serialize();
        file.close();
        logEvent("SYSTEM", "Datele au fost salvate pe disc.");
    }
}

void MainWindow::loadSystemState() {
    std::ifstream file("vfs_data.txt");
    if (file.is_open()) {
        std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (!data.empty()) {
            // Apelăm metoda ta deserialize pentru a reconstrui arborele
            m_root->deserialize(data);
        }
        file.close();
        logEvent("SYSTEM", "Datele au fost încărcate cu succes.");

        // Refresh la interfață
        if (m_model) m_model->refresh();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Înainte să se închidă aplicația, salvăm datele!
    saveSystemState();
    QMainWindow::closeEvent(event);
}



MainWindow::MainWindow(Folder* root,
                       const std::string& username,
                       QWidget* parent)
    : QMainWindow(parent),
    m_root(root),
    m_currentUser(username),
    m_model(nullptr),
    m_searchEngine(),                              // FIX: default constructor
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
    // FIX: initializare PathResolver si Logger in body
    m_pathResolver = new PathResolver(
        std::shared_ptr<Folder>(root, [](Folder*){}));

    m_logger = new TimestampedLogger("logs.txt");

    setWindowTitle(QString("ATMosFS — %1")
                       .arg(QString::fromStdString(username)));
    setMinimumSize(1024, 680);
    setupMenuBar();
    setupUI();
}

void MainWindow::setupMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu("File");
    QAction* newFileAction   = fileMenu->addAction("Fișier nou");
    QAction* newFolderAction = fileMenu->addAction("Folder nou");
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction("Ieșire");

    QMenu* editMenu = menuBar()->addMenu("Edit");
    QAction* renameAction = editMenu->addAction("Redenumire");
    QAction* deleteAction = editMenu->addAction("Șterge");
    renameAction->setShortcut(QKeySequence(Qt::Key_F2));

    QMenu* viewMenu = menuBar()->addMenu("View");
    QAction* sortNameAction = viewMenu->addAction("Sortare după nume");
    QAction* sortSizeAction = viewMenu->addAction("Sortare după dimensiune");
    QAction* sortDateAction = viewMenu->addAction("Sortare după dată");

    QMenu* logMenu = menuBar()->addMenu("Log");
    QAction* showLogsAction   = logMenu->addAction("Afișează log-uri");
    QAction* exportLogsAction = logMenu->addAction("Export log-uri");

    QMenu* helpMenu = menuBar()->addMenu("Help");
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

    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "Despre ATMosFS",
                           "ATMosFS — Sistem de Fișiere Virtual\n"
                           "Proiect POO C++\n\n"
                           "Arhitectură: client-server, SQLite, Qt\n"
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
    m_addressBar->setPlaceholderText("Cale absolută (ex: /root/Documents)");
    m_addressBar->setMinimumHeight(30);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("Caută...");
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
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested,this, &MainWindow::onContextMenu);
    m_treeView->setModel(m_model);
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
    m_previewPane->setPlaceholderText("Selectează un element...");

    auto* resultsLabel = new QLabel("Rezultate căutare", this);
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

void MainWindow::onItemSelected(const QModelIndex& index) {
    if (!index.isValid()) return;

    auto* entity = m_model->entityFromIndex(index);
    if (!entity) return;

    showPreview(entity);
    updateStatusBar(entity);

    // actualizam bara de adresa
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

        QMessageBox::warning(this, "Cale inexistentă",
                             "Calea introdusă nu există în sistemul de fișiere!");
        return;
    }

    m_addressBar->setStyleSheet("");
    logEvent("NAVIGATE", path.toStdString());
}

void MainWindow::onSearchTriggered() {
    QString query = m_searchBar->text().trimmed();
    if (query.isEmpty()) return;

    m_searchResults->clear();

    // FIX: searchByName ia root + query
    auto results = m_searchEngine.searchByName(m_root, query.toStdString());

    if (results.empty()) {
        m_searchResults->addItem("Niciun rezultat pentru: " + query);
        m_statusLabel->setText("Căutare: niciun rezultat");
        return;
    }

    for (const auto& r : results) {
        // FIX: SearchResult are .entity-> si .absolutePath
        QString icon = r.entity->isFolder() ? "📁" : "📄";
        QString name = QString::fromStdString(r.entity->getName());
        QString path = QString::fromStdString(r.absolutePath);

        auto* item = new QListWidgetItem(icon + " " + name + "  —  " + path);
        item->setData(Qt::UserRole, path);
        m_searchResults->addItem(item);
    }

    m_statusLabel->setText(
        QString("Căutare: '%1' — %2 rezultate")
            .arg(query).arg(results.size()));

    logEvent("SEARCH", query.toStdString());
}

void MainWindow::onSearchResultClicked(QListWidgetItem* item) {
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    m_addressBar->setText(path);
}

void MainWindow::onNewFile() {
    auto index = m_treeView->currentIndex();
    Folder* parentFolder = m_root;

    if (index.isValid()) {
        auto* entity = m_model->entityFromIndex(index);
        if (entity && entity->isFolder()) {
            parentFolder = dynamic_cast<Folder*>(entity);
        } else if (entity && !entity->isFolder()) {
            // daca e selectat un fisier luam parintele sau
            auto parentIndex = m_model->parent(index);
            if (parentIndex.isValid()) {
                auto* parent = m_model->entityFromIndex(parentIndex);
                parentFolder = dynamic_cast<Folder*>(parent);
            }
        }
    }

    bool ok;
    QString name = QInputDialog::getText(this,
                                         "Fișier nou", "Numele fișierului (fără extensie):",
                                         QLineEdit::Normal,
                                         QString("Document Nou %1").arg(parentFolder->getChildCount() + 1),
                                         &ok);

    if (!ok || name.isEmpty()) return;

    try {
        // cream fisierul si il adaugam in folder
        auto newFile = std::make_shared<TextFile>(
            name.toStdString(),
            m_currentUser,
            "users",
            ""
            );

        parentFolder->addChild(newFile);

        // refresh model
        m_model->refresh();
        // expandam folderul parinte
        if (index.isValid() && m_model->entityFromIndex(index)->isFolder()) {
            m_treeView->expand(index);
        }

        logEvent("CREATE_FILE", name.toStdString());
        m_statusLabel->setText("Fișier creat: " + name);

    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare",
                             QString::fromStdString(e.what()));
    }
}

void MainWindow::onNewFolder() {
    auto index = m_treeView->currentIndex();
    Folder* parentFolder = m_root;

    if (index.isValid()) {
        auto* entity = m_model->entityFromIndex(index);
        if (entity && entity->isFolder()) {
            parentFolder = dynamic_cast<Folder*>(entity);
        } else if (entity && !entity->isFolder()) {
            auto parentIndex = m_model->parent(index);
            if (parentIndex.isValid()) {
                auto* parent = m_model->entityFromIndex(parentIndex);
                parentFolder = dynamic_cast<Folder*>(parent);
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
        auto newFolder = std::make_shared<Folder>(
            name.toStdString(),
            m_currentUser,
            "users",
            parentFolder
            );

        parentFolder->addChild(newFolder);

        // refresh model
        m_model->refresh();

        if (index.isValid() && m_model->entityFromIndex(index)->isFolder()) {
            m_treeView->expand(index);
        }

        logEvent("CREATE_FOLDER", name.toStdString());
        m_statusLabel->setText("Folder creat: " + name);

    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare",
                             QString::fromStdString(e.what()));
    }
}

void MainWindow::onRename() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;

    auto* entity = m_model->entityFromIndex(index);
    if (!entity) return;

    // Luăm numele curent
    QString currentName = QString::fromStdString(entity->getName());

    // Deschidem pop-up-ul
    bool ok;
    QString newName = QInputDialog::getText(this,
                                            "Redenumire",
                                            "Introdu noul nume:",
                                            QLineEdit::Normal,
                                            currentName,
                                            &ok);

    // Dacă utilizatorul a dat OK, textul nu e gol și e diferit de numele vechi
    if (ok && !newName.isEmpty() && newName != currentName) {
        try {
            // Setăm noul nume
            entity->setName(newName.toStdString());

            // Dăm refresh la model ca să se vadă modificarea în TreeView
            m_model->refresh();

            // Logăm evenimentul
            logEvent("RENAME", currentName.toStdString() + " -> " + newName.toStdString());
            m_statusLabel->setText("Redenumit cu succes în: " + newName);

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
                                       "Confirmare ștergere",
                                       QString("Ești sigur că vrei să ștergi '%1'?")
                                           .arg(QString::fromStdString(entity->getName())),
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    try {
        // gasim parintele
        auto parentIndex = m_model->parent(index);
        Folder* parentFolder = m_root;

        if (parentIndex.isValid()) {
            auto* parent = m_model->entityFromIndex(parentIndex);
            parentFolder = dynamic_cast<Folder*>(parent);
        }

        if (parentFolder) {
            std::string name = entity->getName();
            parentFolder->removeChild(name);

            // refresh model
            m_model->refresh();

            logEvent("DELETE", name);
            m_statusLabel->setText(
                QString("Șters: %1").arg(QString::fromStdString(name)));
        }

    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare",
                             QString::fromStdString(e.what()));
    }
}

void MainWindow::onSortByName() {
    m_currentSort = SortManager::SortCrit::NAME_ASC;
    m_statusLabel->setText("Sortat după: Nume");
    logEvent("SORT", "name");
}

void MainWindow::onSortBySize() {
    m_currentSort = SortManager::SortCrit::SIZE_DESC;
    m_statusLabel->setText("Sortat după: Dimensiune");
    logEvent("SORT", "size");
}

void MainWindow::onSortByDate() {
    m_currentSort = SortManager::SortCrit::DATE_DESC;
    m_statusLabel->setText("Sortat după: Dată");
    logEvent("SORT", "date");
}

void MainWindow::onShowLogs() {
    m_logConsole->show();
}

void MainWindow::onExportLogs() {
    if (m_logger) {
        m_logger->exportLogs("export_logs.txt");
        QMessageBox::information(this, "Export",
                                 "Log-urile au fost exportate în 'export_logs.txt'!");
    }
}

void MainWindow::showPreview(FileSystemEntity* entity) {
    if (!entity) return;
    m_previewPane->clear();

    QString info;
    info += "<b>Nume:</b> " + QString::fromStdString(entity->getName()) + "<br>";
    info += "<b>Proprietar:</b> " + QString::fromStdString(entity->getOwnerUser()) + "<br>";
    info += "<b>Dimensiune:</b> " + QString::number(entity->getSize()) + " bytes<br>";

    std::time_t t = entity->getCreatedAt();
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    info += "<b>Creat la:</b> " + QString(buf) + "<br><hr>";

    if (entity->isFolder()) {
        auto* folder = dynamic_cast<Folder*>(entity);
        info += "<b>Tip:</b> Director<br>";
        info += "<b>Elemente:</b> " +
                QString::number(folder->getChildCount()) + "<br>";
    } else {
        auto* tf = dynamic_cast<TextFile*>(entity);
        if (tf) {
            info += "<b>Tip:</b> Fișier text<br><hr>";
            info += "<pre>" + QString::fromStdString(tf->read()) + "</pre>";
        } else {
            info += "<b>Tip:</b> Fișier binar<br>";
            info += "<i>Conținutul nu poate fi previzualizat.</i>";
        }
    }

    m_previewPane->setHtml(info);
}

void MainWindow::updateStatusBar(FileSystemEntity* entity) {
    if (!entity) return;

    if (entity->isFolder()) {
        auto* folder = dynamic_cast<Folder*>(entity);
        m_statusLabel->setText(               // FIX: m_statusLabel nu m_statusBar
            QString("%1 elemente | %2 B")
                .arg(folder->getChildCount())
                .arg(folder->getSize()));
    } else {
        m_statusLabel->setText(
            QString("1 element | %1 B").arg(entity->getSize()));
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

void MainWindow::onContextMenu(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);

    QMenu contextMenu(this);

    // ── Creare ───────────────────────────────────────
    QAction* newFileAction   = contextMenu.addAction("📄 Fișier nou");
    QAction* newFolderAction = contextMenu.addAction("📁 Folder nou");
    contextMenu.addSeparator();

    // ── Operatii pe element selectat ─────────────────
    QAction* renameAction       = nullptr;
    QAction* deleteAction       = nullptr;
    QAction* shareAction        = nullptr;
    QAction* openExternalAction = nullptr; // Acțiunea nouă

    if (index.isValid()) {
        auto* entity = m_model->entityFromIndex(index);
        if (entity) {
            // Dacă NU e folder, adăugăm butonul de deschidere externă
            if (!entity->isFolder()) {
                openExternalAction = contextMenu.addAction("🖥 Deschide extern (Notepad/Word)");
                contextMenu.addSeparator();
            }

            renameAction = contextMenu.addAction("✏ Redenumire (F2)");
            deleteAction = contextMenu.addAction("🗑 Șterge");
            contextMenu.addSeparator();
            shareAction  = contextMenu.addAction("🔗 Partajare...");
        }
    }

    // ── Executa meniul ───────────────────────────────
    QAction* selected = contextMenu.exec(
        m_treeView->viewport()->mapToGlobal(pos));

    if (!selected) return;

    if (selected == newFileAction)   onNewFile();
    if (selected == newFolderAction) onNewFolder();
    if (renameAction && selected == renameAction) onRename();
    if (deleteAction && selected == deleteAction) onDelete();

    // Declanșează metoda de deschidere dacă a fost selectată
    if (openExternalAction && selected == openExternalAction) {
        onOpenExternal();
    }

    if (shareAction && selected == shareAction) {
        auto* entity = m_model->entityFromIndex(index);
        if (!entity) return;

        bool ok;
        QString user = QInputDialog::getText(this, "Partajare", "Username cu care partajezi:", QLineEdit::Normal, "", &ok);

        if (ok && !user.isEmpty()) {
            try {
                // Cazul 1: Este DEJA un SharedFolder
                auto* sharedFolder = dynamic_cast<SharedFolder*>(entity);
                if (sharedFolder) {
                    sharedFolder->share(user.toStdString());
                    logEvent("SHARE", entity->getName() + " -> " + user.toStdString());
                    QMessageBox::information(this, "Partajare", QString("'%1' a fost partajat și cu '%2'!").arg(QString::fromStdString(entity->getName()), user));
                }
                // Cazul 2: Este un Folder normal. Facem UPGRADE la SharedFolder!
                else if (auto* folder = dynamic_cast<Folder*>(entity)) {
                    Folder* parent = folder->getParent();
                    if (parent) {
                        // Cream noul SharedFolder
                        auto newShared = std::make_shared<SharedFolder>(
                            folder->getName(), folder->getOwnerUser(), folder->getOwnerGroup(), parent
                            );

                        // Mutam toti copiii din vechiul folder in noul SharedFolder
                        for (auto& child : folder->getChildren()) {
                            // Daca copilul e un folder, trebuie sa ii zicem ca si-a schimbat "tatal"
                            if (auto* childFolder = dynamic_cast<Folder*>(child.get())) {
                                childFolder->setParent(newShared.get());
                            }
                            newShared->addChild(child);
                        }

                        // Aplicam partajarea
                        newShared->share(user.toStdString());

                        // Scoatem folderul vechi si il bagam pe cel nou in parinte
                        parent->removeChild(folder->getName());
                        parent->addChild(newShared);

                        m_model->refresh(); // Actualizam UI-ul

                        logEvent("UPGRADE_AND_SHARE", entity->getName() + " -> " + user.toStdString());
                        QMessageBox::information(this, "Partajare Reușită", QString("Folderul '%1' a devenit SharedFolder și a fost partajat cu '%2'!").arg(QString::fromStdString(entity->getName()), user));
                    } else {
                        QMessageBox::warning(this, "Eroare", "Folderul Root nu poate fi partajat!");
                    }
                } else {
                    QMessageBox::warning(this, "Eroare", "Doar folderele pot fi partajate momentan, nu și fișierele!");
                }
            } catch (const std::exception& e) {
                QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
            }
        }
    }
        auto* entity = m_model->entityFromIndex(index);
        if (!entity) return;

        bool ok;
        QString user = QInputDialog::getText(this, "Partajare", "Username cu care partajezi:", QLineEdit::Normal, "", &ok);

        if (ok && !user.isEmpty()) {
            try {
                // Cazul 1: Este DEJA un SharedFolder
                auto* sharedFolder = dynamic_cast<SharedFolder*>(entity);
                if (sharedFolder) {
                    sharedFolder->share(user.toStdString());
                    logEvent("SHARE", entity->getName() + " -> " + user.toStdString());
                    QMessageBox::information(this, "Partajare", QString("'%1' a fost partajat și cu '%2'!").arg(QString::fromStdString(entity->getName()), user));
                }
                // Cazul 2: Este un Folder normal. Facem UPGRADE la SharedFolder!
                else if (auto* folder = dynamic_cast<Folder*>(entity)) {
                    Folder* parent = folder->getParent();
                    if (parent) {
                        // Cream noul SharedFolder
                        auto newShared = std::make_shared<SharedFolder>(
                            folder->getName(), folder->getOwnerUser(), folder->getOwnerGroup(), parent
                            );

                        // Mutam toti copiii din vechiul folder in noul SharedFolder
                        for (auto& child : folder->getChildren()) {
                            // Daca copilul e un folder, trebuie sa ii zicem ca si-a schimbat "tatal"
                            if (auto* childFolder = dynamic_cast<Folder*>(child.get())) {
                                childFolder->setParent(newShared.get());
                            }
                            newShared->addChild(child);
                        }

                        // Aplicam partajarea
                        newShared->share(user.toStdString());

                        // Scoatem folderul vechi si il bagam pe cel nou in parinte
                        parent->removeChild(folder->getName());
                        parent->addChild(newShared);

                        m_model->refresh(); // Actualizam UI-ul

                        logEvent("UPGRADE_AND_SHARE", entity->getName() + " -> " + user.toStdString());
                        QMessageBox::information(this, "Partajare Reușită", QString("Folderul '%1' a devenit SharedFolder și a fost partajat cu '%2'!").arg(QString::fromStdString(entity->getName()), user));
                    } else {
                        QMessageBox::warning(this, "Eroare", "Folderul Root nu poate fi partajat!");
                    }
                } else {
                    QMessageBox::warning(this, "Eroare", "Doar folderele pot fi partajate momentan, nu și fișierele!");
                }
            } catch (const std::exception& e) {
                QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
            }
        }
    }


void FileSystemModel::refresh() {
    beginResetModel();
    endResetModel();
}