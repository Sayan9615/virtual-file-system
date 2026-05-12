#include "MainWindow.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include "../interfaces/IFileManager.h"
#include "../interfaces/IAuthService.h"
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
#include <QShortcut>
#include <fstream>
#include <algorithm>
#include <functional>

static QByteArray createMinimalDocx();
static QByteArray createMinimalXlsx();
static QByteArray createMinimalPptx();
static bool isBinaryOffice(const QString& name);

MainWindow::MainWindow(const std::string& username,
                       IFileManager& fm, IAuthService& auth,
                       QWidget* parent)
    : QMainWindow(parent),
    m_currentUser(username),
    m_fm(fm), m_auth(auth),
    m_model(nullptr), m_searchEngine(),
    m_logger(nullptr),
    m_currentSort(SortManager::SortCrit::NAME_ASC),
    m_treeView(nullptr), m_previewPane(nullptr),
    m_searchBar(nullptr), m_addressBar(nullptr),
    m_searchResults(nullptr), m_mainSplitter(nullptr),
    m_statusLabel(nullptr), m_logConsole(nullptr),
    m_backButton(nullptr)
{
    m_logger = new TimestampedLogger("logs.txt");

    setWindowTitle(QString("ATMosFS — %1%2")
                       .arg(QString::fromStdString(username))
                       .arg(m_auth.isAdmin(username) ? " [ADMIN]" : ""));
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
    logEvent("SYSTEM", "Datele salvate in SQLite.");
}

void MainWindow::loadSystemState() {
    logEvent("SYSTEM", "Sistem incarcat din SQLite.");
}

void MainWindow::setupMenuBar() {
    QMenu* fileMenu = menuBar()->addMenu("File");
    QAction* newFileAction   = fileMenu->addAction("Fisier nou");
    QAction* newFolderAction = fileMenu->addAction("Folder nou");
    fileMenu->addSeparator();
    QAction* editFileAction  = fileMenu->addAction("Editeaza fisier (Enter)");
    editFileAction->setShortcut(QKeySequence(Qt::Key_Return));
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction("Iesire");

    QMenu* editMenu = menuBar()->addMenu("Edit");
    QAction* renameAction     = editMenu->addAction("Redenumire");
    QAction* deleteAction     = editMenu->addAction("Sterge");
    editMenu->addSeparator();
    QAction* shareMenuAction  = editMenu->addAction("Partajare...");
    QAction* propertiesAction = editMenu->addAction("Proprietati");
    renameAction->setShortcut(QKeySequence(Qt::Key_F2));
    propertiesAction->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Return));

    QMenu* viewMenu = menuBar()->addMenu("View");
    QAction* sortNameAction = viewMenu->addAction("Sortare dupa nume");
    QAction* sortSizeAction = viewMenu->addAction("Sortare dupa dimensiune");
    QAction* sortDateAction = viewMenu->addAction("Sortare dupa data");

    QMenu* logMenu = menuBar()->addMenu("Log");
    QAction* showLogsAction   = logMenu->addAction("Afiseaza log-uri");
    QAction* exportLogsAction = logMenu->addAction("Export log-uri");

    if (m_auth.isAdmin(m_currentUser)) {
        QMenu* adminMenu = menuBar()->addMenu("⚙ Admin");
        QAction* adminPanelAction = adminMenu->addAction("Panou Administrare");
        connect(adminPanelAction, &QAction::triggered,
                this, &MainWindow::onAdminPanel);
    }

    QMenu* helpMenu = menuBar()->addMenu("Help");
    QAction* aboutAction = helpMenu->addAction("Despre ATMosFS");

    connect(exitAction,       &QAction::triggered, qApp, &QApplication::quit);
    connect(newFileAction,    &QAction::triggered, this, &MainWindow::onNewFile);
    connect(newFolderAction,  &QAction::triggered, this, &MainWindow::onNewFolder);
    connect(editFileAction,   &QAction::triggered, this, &MainWindow::onEditFile);
    connect(renameAction,     &QAction::triggered, this, &MainWindow::onRename);
    connect(deleteAction,     &QAction::triggered, this, &MainWindow::onDelete);
    connect(shareMenuAction,  &QAction::triggered, this, &MainWindow::onShareDialog);
    connect(propertiesAction, &QAction::triggered, this, &MainWindow::onProperties);
    connect(sortNameAction,   &QAction::triggered, this, &MainWindow::onSortByName);
    connect(sortSizeAction,   &QAction::triggered, this, &MainWindow::onSortBySize);
    connect(sortDateAction,   &QAction::triggered, this, &MainWindow::onSortByDate);
    connect(showLogsAction,   &QAction::triggered, this, &MainWindow::onShowLogs);
    connect(exportLogsAction, &QAction::triggered, this, &MainWindow::onExportLogs);
    connect(aboutAction, &QAction::triggered, this, [this]() {
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

    // ── Top bar ───────────────────────────────────────
    auto* topBar    = new QWidget(this);
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(8, 6, 8, 6);

    m_backButton = new QPushButton("← Inapoi", this);
    m_backButton->setEnabled(false);
    m_backButton->setFixedHeight(30);

    m_addressBar = new QLineEdit(this);
    m_addressBar->setPlaceholderText("Cale curenta");
    m_addressBar->setReadOnly(true);
    m_addressBar->setMinimumHeight(30);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("Cauta...");
    m_searchBar->setMinimumHeight(30);
    m_searchBar->setMaximumWidth(250);

    auto* searchBtn = new QPushButton("🔍", this);
    searchBtn->setFixedSize(32, 30);

    topLayout->addWidget(m_backButton);
    topLayout->addWidget(new QLabel("📁", this));
    topLayout->addWidget(m_addressBar, 1);
    topLayout->addWidget(m_searchBar);
    topLayout->addWidget(searchBtn);

    // ── Model si TreeView ─────────────────────────────
    m_model = new FileSystemModel(m_fm, m_currentUser, m_fm.getRootId(), this);

    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_model);
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_treeView->setAlternatingRowColors(true);
    m_treeView->setAnimated(true);
    m_treeView->setSortingEnabled(false);
    m_treeView->header()->setSectionsClickable(true);
    m_treeView->header()->setSortIndicatorShown(true);
    m_treeView->setRootIsDecorated(false); // model plat, fara expand arrows

    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeView->header()->resizeSection(1, 80);
    m_treeView->header()->resizeSection(2, 100);
    m_treeView->header()->resizeSection(3, 140);
    m_treeView->setMinimumWidth(400);

    // ── Right panel ───────────────────────────────────
    auto* rightPanel  = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(8, 8, 8, 8);

    QFont boldFont; boldFont.setBold(true);

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

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->addWidget(m_treeView);
    m_mainSplitter->addWidget(rightPanel);
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 2);

    m_statusLabel = new QLabel("0 elemente", this);
    m_statusLabel->setStyleSheet(
        "padding: 4px 10px; background-color: #2B2B2B; "
        "color: #B0B0B0; border-top: 1px solid #444444; font-size: 11px;");

    mainLayout->addWidget(topBar);
    mainLayout->addWidget(m_mainSplitter, 1);
    mainLayout->addWidget(m_statusLabel);
    setCentralWidget(centralWidget);

    // ── Connections ───────────────────────────────────
    connect(m_backButton, &QPushButton::clicked, this, [this]() {
        m_model->goBack();
        m_backButton->setEnabled(m_model->canGoBack());
        m_addressBar->setText(QString("Folder id: %1").arg(m_model->currentFolderId()));
        m_statusLabel->setText(QString("%1 elemente").arg(m_model->rowCount()));
    });

    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &MainWindow::onContextMenu);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::onItemSelected);
    connect(searchBtn, &QPushButton::clicked,
            this, &MainWindow::onSearchTriggered);
    connect(m_searchBar, &QLineEdit::returnPressed,
            this, &MainWindow::onSearchTriggered);
    connect(m_searchResults, &QListWidget::itemClicked,
            this, &MainWindow::onSearchResultClicked);

    // dublu click — intra in folder sau editeaza fisier
    connect(m_treeView, &QTreeView::doubleClicked,
            this, [this](const QModelIndex& index) {
                if (!index.isValid()) return;
                auto* entity = m_model->entityFromIndex(index);
                if (!entity) return;

                if (entity->isFolder()) {
                    int folderId = entity->getId();
                    QString folderName = QString::fromStdString(entity->getName());
                    m_model->navigateTo(folderId);
                    m_backButton->setEnabled(m_model->canGoBack());
                    m_addressBar->setText(folderName);
                    m_statusLabel->setText(
                        QString("%1 elemente").arg(m_model->rowCount()));
                } else {
                    showEditDialog(entity);
                }
            });

    m_addressBar->setText("/ (root)");
    m_statusLabel->setText(QString("%1 elemente").arg(m_model->rowCount()));
}

// ── Permisiuni ────────────────────────────────────────

bool MainWindow::checkWritePermission(FileSystemEntity* entity) {
    if (!entity) return false;
    if (entity->getId() <= 1) return true;
    if (m_auth.isAdmin(m_currentUser)) return true;
    if (entity->getOwnerUser() == m_currentUser) return true;

    if (!m_fm.checkPermission(entity->getId(), m_currentUser, "write")) {
        QMessageBox::warning(this, "Acces refuzat",
                             QString("Nu aveti permisiunea de scriere asupra '%1'!")
                                 .arg(QString::fromStdString(entity->getName())));
        return false;
    }
    return true;
}

bool MainWindow::checkReadPermission(FileSystemEntity* entity) {
    if (!entity) return false;
    if (entity->getId() <= 1) return true;
    if (m_auth.isAdmin(m_currentUser)) return true;
    if (entity->getOwnerUser() == m_currentUser) return true;

    if (!m_fm.checkPermission(entity->getId(), m_currentUser, "read")) {
        QMessageBox::warning(this, "Acces refuzat",
                             QString("Nu aveti permisiunea de citire asupra '%1'!")
                                 .arg(QString::fromStdString(entity->getName())));
        return false;
    }
    return true;
}

// ── Admin Panel ───────────────────────────────────────

void MainWindow::onAdminPanel() {
    if (!m_auth.isAdmin(m_currentUser)) {
        QMessageBox::warning(this, "Acces refuzat",
                             "Doar administratorul poate accesa acest panou!");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("⚙ Panou Administrare — ATMosFS");
    dialog.setFixedSize(420, 500);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    layout->addWidget(new QLabel("<b>Utilizatori inregistrati:</b>", &dialog));

    auto* userList = new QListWidget(&dialog);
    auto allUsers  = m_auth.getAllUsers();

    for (const auto& [id, uname] : allUsers) {
        QString display = QString("[%1] %2").arg(id).arg(QString::fromStdString(uname));
        if (uname == "admin") display += "  🛡 (admin)";
        auto* item = new QListWidgetItem(display, userList);
        item->setData(Qt::UserRole, QString::fromStdString(uname));
        if (uname == "admin") item->setForeground(QColor(0, 120, 215));
    }

    layout->addWidget(userList);

    auto* infoLabel = new QLabel("Selecteaza un utilizator pentru actiuni.", &dialog);
    infoLabel->setStyleSheet("color: #555; font-size: 11px;");
    layout->addWidget(infoLabel);

    auto* btnLayout     = new QHBoxLayout();
    auto* deleteUserBtn = new QPushButton("🗑 Sterge User", &dialog);
    auto* closeBtn      = new QPushButton("Inchide", &dialog);

    deleteUserBtn->setStyleSheet(
        "QPushButton { background-color: #D32F2F; color: white; "
        "border-radius: 4px; font-weight: bold; padding: 6px 12px; }"
        "QPushButton:hover { background-color: #B71C1C; }"
        "QPushButton:disabled { background-color: #CCCCCC; color: #888888; }");
    deleteUserBtn->setEnabled(false);

    btnLayout->addWidget(deleteUserBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    connect(userList, &QListWidget::itemSelectionChanged, &dialog, [&]() {
        auto selected = userList->selectedItems();
        if (selected.isEmpty()) {
            deleteUserBtn->setEnabled(false);
            infoLabel->setText("Selecteaza un utilizator pentru actiuni.");
            return;
        }
        QString uname = selected[0]->data(Qt::UserRole).toString();
        if (uname == "admin") {
            deleteUserBtn->setEnabled(false);
            infoLabel->setText("Contul admin nu poate fi sters.");
        } else {
            deleteUserBtn->setEnabled(true);
            infoLabel->setText(QString("Selectat: <b>%1</b>").arg(uname));
        }
    });

    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    connect(deleteUserBtn, &QPushButton::clicked, &dialog, [&]() {
        auto selected = userList->selectedItems();
        if (selected.isEmpty()) return;
        QString uname = selected[0]->data(Qt::UserRole).toString();
        if (uname == "admin") {
            QMessageBox::warning(&dialog, "Eroare", "Contul admin nu poate fi sters!");
            return;
        }
        auto reply = QMessageBox::question(&dialog, "Confirmare stergere user",
                                           QString("Esti sigur ca vrei sa stergi userul '%1'?").arg(uname),
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
        if (m_auth.deleteUser(uname.toStdString())) {
            delete selected[0];
            infoLabel->setText(QString("Userul '%1' a fost sters.").arg(uname));
            deleteUserBtn->setEnabled(false);
            logEvent("ADMIN_DELETE_USER", uname.toStdString());
            QMessageBox::information(&dialog, "Succes",
                                     QString("Userul '%1' a fost sters!").arg(uname));
        } else {
            QMessageBox::warning(&dialog, "Eroare", "Nu s-a putut sterge userul!");
        }
    });

    dialog.exec();
}

// ── Navigare ─────────────────────────────────────────

void MainWindow::onItemSelected(const QModelIndex& index) {
    if (!index.isValid()) return;
    auto* entity = m_model->entityFromIndex(index);
    if (!entity) return;

    showPreview(entity);
    updateStatusBar(entity);
}

// ── Cautare ──────────────────────────────────────────

void MainWindow::onSearchTriggered() {
    QString query = m_searchBar->text().trimmed();
    if (query.isEmpty()) return;

    m_searchResults->clear();

    // pentru search construim arborele din folderul curent
    auto root = std::dynamic_pointer_cast<Folder>(
        std::shared_ptr<Folder>(new Folder("search_root", "system")));

    // folosim buildTree din folderul curent pentru search
    auto tree = m_fm.buildTree(m_model->currentFolderId(), nullptr, m_currentUser);
    if (!tree) {
        m_searchResults->addItem("Niciun rezultat.");
        return;
    }

    auto results = m_searchEngine.search(tree.get(), query.toStdString(), true, true);

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
        item->setData(Qt::UserRole, (int)r.entity->getId());
        m_searchResults->addItem(item);
    }

    m_statusLabel->setText(QString("Cautare: '%1' — %2 rezultate")
                               .arg(query).arg(results.size()));
    logEvent("SEARCH", query.toStdString());
}

void MainWindow::onSearchResultClicked(QListWidgetItem* item) {
    if (!item) return;
    int entityId = item->data(Qt::UserRole).toInt();
    // navigam la parintele entitatii
    int parentId = m_fm.getParentId(entityId);
    if (parentId > 0) {
        m_model->navigateTo(parentId);
        m_backButton->setEnabled(m_model->canGoBack());
        m_addressBar->setText(QString("Folder id: %1").arg(parentId));
    }
}

// ── Creare fisier ─────────────────────────────────────

void MainWindow::onNewFile() {
    int parentId = m_model->currentFolderId();

    QDialog typeDialog(this);
    typeDialog.setWindowTitle("Fisier nou");
    typeDialog.setFixedSize(380, 160);
    auto* dlgLayout  = new QVBoxLayout(&typeDialog);
    auto* formLayout = new QFormLayout();

    auto* typeCombo = new QComboBox(&typeDialog);
    typeCombo->addItem("Document Text (.txt)");
    typeCombo->addItem("Document Word (.docx)");
    typeCombo->addItem("Registru Excel (.xlsx)");
    typeCombo->addItem("Prezentare PowerPoint (.pptx)");
    const QStringList typeExts = {"txt","docx","xlsx","pptx"};

    auto* nameEdit = new QLineEdit(&typeDialog);
    nameEdit->setText("Document_nou.txt");

    connect(typeCombo, &QComboBox::currentIndexChanged, &typeDialog, [&](int idx) {
        if (idx < 0 || idx >= typeExts.size()) return;
        QString ext  = typeExts[idx];
        QString base = nameEdit->text().trimmed();
        int dot = base.lastIndexOf('.');
        if (dot >= 0) base = base.left(dot);
        if (base.isEmpty()) base = "Document_nou";
        nameEdit->setText(base + "." + ext);
    });

    formLayout->addRow("Tip fisier:", typeCombo);
    formLayout->addRow("Nume:", nameEdit);
    dlgLayout->addLayout(formLayout);

    auto* btnRow  = new QHBoxLayout();
    auto* cancelB = new QPushButton("Anuleaza", &typeDialog);
    auto* createB = new QPushButton("Creeaza",  &typeDialog);
    createB->setDefault(true);
    btnRow->addStretch();
    btnRow->addWidget(cancelB);
    btnRow->addWidget(createB);
    dlgLayout->addLayout(btnRow);
    connect(cancelB, &QPushButton::clicked, &typeDialog, &QDialog::reject);
    connect(createB, &QPushButton::clicked, &typeDialog, &QDialog::accept);

    if (typeDialog.exec() != QDialog::Accepted) return;

    QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) return;

    std::string initialContent = " ";
    {
        QByteArray tpl;
        if (name.endsWith(".docx", Qt::CaseInsensitive))      tpl = createMinimalDocx();
        else if (name.endsWith(".xlsx", Qt::CaseInsensitive)) tpl = createMinimalXlsx();
        else if (name.endsWith(".pptx", Qt::CaseInsensitive)) tpl = createMinimalPptx();
        if (!tpl.isEmpty()) initialContent = tpl.toBase64().toStdString();
    }

    try {
        bool saved = m_fm.createTextFile(name.toStdString(), m_currentUser, initialContent, parentId);
        if (!saved) {
            QMessageBox::warning(this, "Eroare", "Eroare la salvare in baza de date.");
            return;
        }
        m_model->refresh();
        logEvent("CREATE_FILE", name.toStdString());
        m_statusLabel->setText("Fisier creat: " + name);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

// ── Creare folder ─────────────────────────────────────

void MainWindow::onNewFolder() {
    int parentId = m_model->currentFolderId();

    bool ok;
    QString name = QInputDialog::getText(this, "Folder nou", "Numele folderului:",
                                         QLineEdit::Normal, "Folder_nou", &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed();

    try {
        bool saved = m_fm.createFolder(name.toStdString(), m_currentUser, parentId);
        if (!saved) {
            QMessageBox::warning(this, "Eroare", "Eroare la salvare in baza de date.");
            return;
        }
        m_model->refresh();
        logEvent("CREATE_FOLDER", name.toStdString());
        m_statusLabel->setText("Folder creat: " + name);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

// ── Editare ───────────────────────────────────────────

void MainWindow::onEditFile() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    auto* entity = m_model->entityFromIndex(index);
    if (entity && !entity->isFolder()) showEditDialog(entity);
}

void MainWindow::showEditDialog(FileSystemEntity* entity) {
    if (!entity || entity->isFolder()) return;
    std::string name = entity->getName();

    if (name.find(".docx") != std::string::npos ||
        name.find(".pptx") != std::string::npos ||
        name.find(".xlsx") != std::string::npos ||
        name.find(".pdf")  != std::string::npos) {
        onOpenExternal(entity);
        return;
    }

    auto* tf = dynamic_cast<TextFile*>(entity);
    if (!tf) { onOpenExternal(entity); return; }
    if (!checkWritePermission(entity)) return;

    QDialog dialog(this);
    dialog.setWindowTitle(QString("Editeaza — %1").arg(QString::fromStdString(name)));
    dialog.setMinimumSize(600, 400);

    auto* layout = new QVBoxLayout(&dialog);
    auto* editor = new QTextEdit(&dialog);
    editor->setPlainText(QString::fromStdString(tf->read()));
    editor->setFont(QFont("Consolas", 11));
    layout->addWidget(editor, 1);

    auto* charCount = new QLabel(QString("Caractere: %1").arg(tf->read().size()), &dialog);
    charCount->setStyleSheet("color: #888; font-size: 10px;");
    layout->addWidget(charCount);

    connect(editor, &QTextEdit::textChanged, &dialog, [&]() {
        charCount->setText(QString("Caractere: %1").arg(editor->toPlainText().length()));
    });

    auto* btnLayout = new QHBoxLayout();
    auto* cancelBtn = new QPushButton("Anuleaza", &dialog);
    auto* saveBtn   = new QPushButton("💾 Salveaza", &dialog);
    saveBtn->setStyleSheet(
        "QPushButton { background-color: #0078D4; color: white; "
        "border-radius: 4px; font-weight: bold; padding: 6px 16px; }"
        "QPushButton:hover { background-color: #006CBE; }");
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    layout->addLayout(btnLayout);

    auto* sc = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), &dialog);
    connect(sc, &QShortcut::activated, saveBtn, &QPushButton::click);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    connect(saveBtn, &QPushButton::clicked, &dialog, [&]() {
        QString newContent = editor->toPlainText();
        try {
            if (m_fm.updateTextFile(entity->getId(), newContent.toStdString(), m_currentUser)) {
                tf->write(newContent.toStdString());
                showPreview(entity);
                m_model->refresh();
                logEvent("EDIT_FILE", entity->getName());
                dialog.accept();
                m_statusLabel->setText(QString("Salvat: %1")
                    .arg(QString::fromStdString(entity->getName())));
            } else {
                QMessageBox::warning(&dialog, "Eroare", "Eroare la salvare in baza de date.");
            }
        } catch (const std::exception& e) {
            QMessageBox::warning(&dialog, "Eroare", e.what());
        }
    });

    dialog.exec();
}

// ── Open External ─────────────────────────────────────

void MainWindow::onOpenExternal(FileSystemEntity* entityOverride) {
    FileSystemEntity* entity = entityOverride;
    if (!entity) {
        auto index = m_treeView->currentIndex();
        if (!index.isValid()) return;
        entity = m_model->entityFromIndex(index);
    }
    if (!entity || entity->isFolder()) return;
    if (!checkReadPermission(entity)) return;

    auto* tf = dynamic_cast<TextFile*>(entity);
    if (!tf) return;

    try {
        QString fileName = QString::fromStdString(entity->getName());
        bool binary = isBinaryOffice(fileName);

        QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                           + "/" + fileName;
        QFile realFile(tempPath);

        if (!realFile.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, "Eroare", "Nu s-a putut crea fisierul temporar!");
            return;
        }

        std::string stored = tf->read();
        if (binary) {
            QByteArray raw = QByteArray::fromBase64(
                QString::fromStdString(stored).trimmed().toUtf8());
            if (raw.isEmpty()) {
                if (fileName.endsWith(".docx", Qt::CaseInsensitive))      raw = createMinimalDocx();
                else if (fileName.endsWith(".xlsx", Qt::CaseInsensitive)) raw = createMinimalXlsx();
                else if (fileName.endsWith(".pptx", Qt::CaseInsensitive)) raw = createMinimalPptx();
            }
            if (!raw.isEmpty()) realFile.write(raw);
        } else {
            realFile.write(QByteArray::fromStdString(stored));
        }
        realFile.close();

        QDesktopServices::openUrl(QUrl::fromLocalFile(tempPath));
        logEvent("OPEN_EXTERNAL", entity->getName());

        int idToCheck = entity->getId() <= 0 ? 1 : entity->getId();
        bool canWrite = m_auth.isAdmin(m_currentUser) ||
                        entity->getOwnerUser() == m_currentUser ||
                        m_fm.checkPermission(idToCheck, m_currentUser, "write");

        if (canWrite) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Sincronizare");
            msgBox.setText(QString("Documentul '%1' a fost deschis.\n\n"
                                   "Editeaza, salveaza (Ctrl+S),\n"
                                   "inchide aplicatia, apoi apasa OK.").arg(fileName));
            msgBox.setIcon(QMessageBox::Information);
            msgBox.exec();

            if (realFile.open(QIODevice::ReadOnly)) {
                QByteArray newData = realFile.readAll();
                realFile.close();

                std::string toStore = binary
                    ? newData.toBase64().toStdString()
                    : std::string(newData.constData(), newData.size());

                if (m_fm.updateTextFile(entity->getId(), toStore, m_currentUser)) {
                    tf->write(toStore);
                    m_model->refresh();
                    showPreview(entity);
                    m_statusLabel->setText("Sincronizare reusita: " + fileName);
                    logEvent("UPDATE_FROM_EXTERNAL", entity->getName());
                } else {
                    QMessageBox::warning(this, "Eroare DB",
                                         "Modificarile nu au putut fi salvate.");
                }
            }
        }
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

// ── Redenumire ────────────────────────────────────────

void MainWindow::onRename() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    auto* entity = m_model->entityFromIndex(index);
    if (!entity || !checkWritePermission(entity)) return;

    QString currentName = QString::fromStdString(entity->getName());
    bool ok;
    QString newName = QInputDialog::getText(this, "Redenumire", "Introdu noul nume:",
                                            QLineEdit::Normal, currentName, &ok);

    if (ok && !newName.isEmpty() && newName != currentName) {
        try {
            if (m_fm.renameEntity(entity->getId(), newName.toStdString(), m_currentUser)) {
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
    if (!entity || !checkWritePermission(entity)) return;

    auto reply = QMessageBox::question(this, "Confirmare stergere",
                                       QString("Esti sigur ca vrei sa stergi '%1'?")
                                           .arg(QString::fromStdString(entity->getName())),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    try {
        m_fm.deleteEntity(entity->getId(), m_currentUser);
        m_model->refresh();
        logEvent("DELETE", entity->getName());
        m_statusLabel->setText(QString("Sters: %1")
            .arg(QString::fromStdString(entity->getName())));
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

// ── Context Menu ──────────────────────────────────────

void MainWindow::onContextMenu(const QPoint& pos) {
    QModelIndex index = m_treeView->indexAt(pos);
    QMenu contextMenu(this);

    QAction* newFileAction   = contextMenu.addAction("📄 Fisier nou aici");
    QAction* newFolderAction = contextMenu.addAction("📁 Folder nou aici");
    contextMenu.addSeparator();

    QAction* openExternalAction = nullptr;
    QAction* editAction         = nullptr;
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
                editAction = contextMenu.addAction("📝 Editeaza Text");
                contextMenu.addSeparator();
            }
            renameAction = contextMenu.addAction("✏ Redenumire (F2)");
            deleteAction = contextMenu.addAction("🗑 Sterge");

            int idToCheck = entity->getId() <= 0 ? 1 : entity->getId();
            bool canWrite = m_auth.isAdmin(m_currentUser) ||
                            entity->getOwnerUser() == m_currentUser ||
                            m_fm.checkPermission(idToCheck, m_currentUser, "write");
            if (!canWrite) {
                deleteAction->setEnabled(false);
                deleteAction->setToolTip("Nu aveti permisiunea de scriere!");
            }

            contextMenu.addSeparator();
            shareAction = contextMenu.addAction("🔗 Partajare...");
            if (entity->getOwnerUser() != m_currentUser && !m_auth.isAdmin(m_currentUser)) {
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
    if (editAction && selected == editAction) { showEditDialog(entity); return; }
    if (renameAction && selected == renameAction) { onRename(); return; }
    if (deleteAction && selected == deleteAction) { onDelete(); return; }
    if (openExternalAction && selected == openExternalAction) { onOpenExternal(); return; }
    if (shareAction && selected == shareAction) { showShareDialog(entity); return; }
    if (propertiesAction && selected == propertiesAction) { showPropertiesDialog(entity); return; }
}

// ── Share Dialog ──────────────────────────────────────

void MainWindow::onShareDialog() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    auto* entity = m_model->entityFromIndex(index);
    if (entity) showShareDialog(entity);
}

void MainWindow::showShareDialog(FileSystemEntity* entity) {
    if (!entity) return;

    bool canShare = entity->getOwnerUser() == m_currentUser || m_auth.isAdmin(m_currentUser);
    if (!canShare) {
        QMessageBox::warning(this, "Acces refuzat",
                             "Doar proprietarul sau adminul poate partaja!");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QString("Partajare — %1")
                              .arg(QString::fromStdString(entity->getName())));
    dialog.setFixedSize(380, 420);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    layout->addWidget(new QLabel(
        QString("<b>Partajeaza '%1' cu:</b>")
            .arg(QString::fromStdString(entity->getName())), &dialog));

    auto* listWidget = new QListWidget(&dialog);
    listWidget->setSelectionMode(QAbstractItemView::NoSelection);

    auto allUsers = m_auth.getAllUsers();
    auto pm = m_fm.loadPermissions(entity->getId());
    auto up = pm.getUserPermission();

    for (const auto& [id, uname] : allUsers) {
        if (uname == m_currentUser) continue;
        auto* item = new QListWidgetItem(
            QString("👤 ") + QString::fromStdString(uname), listWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        bool isShared = up && up->hasUser(uname);
        item->setCheckState(isShared ? Qt::Checked : Qt::Unchecked);
        if (isShared) item->setForeground(QColor(0, 120, 215));
        item->setData(Qt::UserRole, QString::fromStdString(uname));
    }

    if (listWidget->count() == 0)
        listWidget->addItem("Nu exista alti utilizatori inregistrati.");

    layout->addWidget(listWidget);

    auto* permGroup  = new QGroupBox("Nivel acces", &dialog);
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
    applyBtn->setStyleSheet(
        "QPushButton { background-color: #0078D4; color: white; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #006CBE; }");
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(applyBtn);
    layout->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    connect(applyBtn, &QPushButton::clicked, &dialog, [&]() {
        for (int i = 0; i < listWidget->count(); i++) {
            auto* item = listWidget->item(i);
            if (!item) continue;
            std::string uname = item->data(Qt::UserRole).toString().toStdString();
            if (uname.empty()) continue;

            bool isChecked = (item->checkState() == Qt::Checked);
            bool wasShared = up && up->hasUser(uname);

            if (isChecked && !wasShared) {
                try {
                    m_fm.shareEntity(entity->getId(), uname,
                                     canReadCheck->isChecked(),
                                     canWriteCheck->isChecked());
                } catch (...) {}
            } else if (!isChecked && wasShared) {
                try {
                    m_fm.revokeShare(entity->getId(), uname);
                } catch (...) {}
            }
        }
        logEvent("SHARE", entity->getName());
        dialog.accept();
    });

    if (dialog.exec() == QDialog::Accepted) {
        m_model->refresh();
        QMessageBox::information(this, "Partajare", "Setarile de partajare au fost salvate!");
    }
}

// ── Proprietati ───────────────────────────────────────

void MainWindow::onProperties() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    auto* entity = m_model->entityFromIndex(index);
    if (entity) showPropertiesDialog(entity);
}

void MainWindow::showPropertiesDialog(FileSystemEntity* entity) {
    if (!entity) return;

    QDialog dialog(this);
    dialog.setWindowTitle(QString("Proprietati — %1")
                              .arg(QString::fromStdString(entity->getName())));
    dialog.setFixedSize(360, 380);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* infoGroup  = new QGroupBox("Informatii", &dialog);
    auto* infoLayout = new QFormLayout(infoGroup);
    infoLayout->addRow("Nume:",       new QLabel(QString::fromStdString(entity->getName())));
    infoLayout->addRow("Tip:",        new QLabel(entity->isFolder() ? "Director" : "Fisier"));
    infoLayout->addRow("Proprietar:", new QLabel(QString::fromStdString(entity->getOwnerUser())));
    infoLayout->addRow("Dimensiune:", new QLabel(QString::number(entity->getSize()) + " bytes"));

    std::time_t t = entity->getCreatedAt();
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    infoLayout->addRow("Creat la:", new QLabel(QString(buf)));
    layout->addWidget(infoGroup);

    bool isOwner = entity->getOwnerUser() == m_currentUser || m_auth.isAdmin(m_currentUser);

    auto* permGroup  = new QGroupBox("Permisiuni", &dialog);
    auto* permLayout = new QVBoxLayout(permGroup);
    permLayout->addWidget(new QLabel("<b>Owner:</b> Read + Write (implicit)"));
    permLayout->addSpacing(6);
    permLayout->addWidget(new QLabel("<b>Others:</b>"));
    auto* othersReadCheck  = new QCheckBox("Poate Citi",     permGroup);
    auto* othersWriteCheck = new QCheckBox("Poate Modifica", permGroup);
    permLayout->addWidget(othersReadCheck);
    permLayout->addWidget(othersWriteCheck);

    try {
        auto pm = m_fm.loadPermissions(entity->getId());
        auto op = pm.getOthersPermission();
        if (op) {
            othersReadCheck->setChecked(op->canRead());
            othersWriteCheck->setChecked(op->canWrite());
        }
    } catch (...) {}

    if (!isOwner) {
        othersReadCheck->setEnabled(false);
        othersWriteCheck->setEnabled(false);
    }

    layout->addWidget(permGroup);

    auto* btnLayout = new QHBoxLayout();
    auto* closeBtn  = new QPushButton("Inchide", &dialog);
    auto* applyBtn  = new QPushButton("Aplica",  &dialog);
    applyBtn->setEnabled(isOwner);
    applyBtn->setStyleSheet(
        "QPushButton { background-color: #0078D4; color: white; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:disabled { background-color: #CCC; color: #888; }");
    btnLayout->addWidget(closeBtn);
    btnLayout->addWidget(applyBtn);
    layout->addLayout(btnLayout);

    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(applyBtn, &QPushButton::clicked, &dialog, [&]() {
        try {
            m_fm.updateOthersPermissions(entity->getId(),
                                         othersReadCheck->isChecked(),
                                         othersWriteCheck->isChecked());
            logEvent("PERMISSIONS", entity->getName());
            dialog.accept();
            QMessageBox::information(this, "Permisiuni", "Permisiunile au fost actualizate!");
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
        }
    });

    dialog.exec();
}

// ── Preview ───────────────────────────────────────────

void MainWindow::showPreview(FileSystemEntity* entity) {
    if (!entity) return;
    m_previewPane->clear();

    QString info;
    info += "<b>Nume:</b> " + QString::fromStdString(entity->getName()) + "<br>";
    info += "<b>Proprietar:</b> " + QString::fromStdString(entity->getOwnerUser());
    if (m_auth.isAdmin(entity->getOwnerUser())) info += " 🛡";
    info += "<br>";
    info += "<b>Dimensiune:</b> " + QString::number(entity->getSize()) + " bytes<br>";

    std::time_t t = entity->getCreatedAt();
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    info += "<b>Creat la:</b> " + QString(buf) + "<br><hr>";

    if (entity->isFolder()) {
        info += "<b>Tip:</b> Director<br>";
        info += "<i>Dublu click pentru a intra in folder.</i>";
    } else {
        auto* tf = dynamic_cast<TextFile*>(entity);
        if (tf) {
            QString name = QString::fromStdString(entity->getName());
            QString docType = "Fisier text";
            if (name.endsWith(".docx", Qt::CaseInsensitive))      docType = "Document Word (.docx)";
            else if (name.endsWith(".pptx", Qt::CaseInsensitive)) docType = "Prezentare PowerPoint (.pptx)";
            else if (name.endsWith(".xlsx", Qt::CaseInsensitive)) docType = "Registru Excel (.xlsx)";

            info += "<b>Tip:</b> " + docType + "<br><hr>";

            if (isBinaryOffice(name)) {
                QByteArray raw = QByteArray::fromBase64(QByteArray::fromStdString(tf->read()));
                info += QString("<i>Fisier binar — %1 bytes stocati.</i>").arg(raw.size());
            } else {
                info += "<pre>" + QString::fromStdString(tf->read()).toHtmlEscaped() + "</pre>";
            }
        } else {
            info += "<b>Tip:</b> Fisier binar<br>";
            info += "<i>Nu poate fi previzualizat.</i>";
        }
    }

    m_previewPane->setHtml(info);
}

void MainWindow::updateStatusBar(FileSystemEntity* entity) {
    if (!entity) return;
    m_statusLabel->setText(entity->isFolder()
        ? QString("Director — %1 bytes").arg(entity->getSize())
        : QString("Fisier — %1 bytes").arg(entity->getSize()));
}

void MainWindow::logEvent(const std::string& action, const std::string& path) {
    if (m_logger) {
        std::string msg = "[" + m_currentUser + "] " + action;
        if (!path.empty()) msg += " -> " + path;
        m_logger->log(msg);
        if (m_logConsole) m_logConsole->append(QString::fromStdString(msg));
    }
}

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

void MainWindow::onSortByName() {
    Qt::SortOrder next =
        (m_treeView->header()->sortIndicatorSection() == 0 &&
         m_treeView->header()->sortIndicatorOrder() == Qt::AscendingOrder)
            ? Qt::DescendingOrder : Qt::AscendingOrder;
    m_treeView->header()->setSortIndicator(0, next);
}

void MainWindow::onSortBySize() {
    Qt::SortOrder next =
        (m_treeView->header()->sortIndicatorSection() == 2 &&
         m_treeView->header()->sortIndicatorOrder() == Qt::AscendingOrder)
            ? Qt::DescendingOrder : Qt::AscendingOrder;
    m_treeView->header()->setSortIndicator(2, next);
}

void MainWindow::onSortByDate() {
    Qt::SortOrder next =
        (m_treeView->header()->sortIndicatorSection() == 3 &&
         m_treeView->header()->sortIndicatorOrder() == Qt::AscendingOrder)
            ? Qt::DescendingOrder : Qt::AscendingOrder;
    m_treeView->header()->setSortIndicator(3, next);
}

static bool isBinaryOffice(const QString& name) {
    return name.endsWith(".docx", Qt::CaseInsensitive) ||
           name.endsWith(".pptx", Qt::CaseInsensitive) ||
           name.endsWith(".xlsx", Qt::CaseInsensitive);
}

static void zipU16(QByteArray& ba, quint16 v) {
    ba.append(char(v&0xFF)); ba.append(char((v>>8)&0xFF));
}
static void zipU32(QByteArray& ba, quint32 v) {
    ba.append(char(v&0xFF)); ba.append(char((v>>8)&0xFF));
    ba.append(char((v>>16)&0xFF)); ba.append(char((v>>24)&0xFF));
}
static quint32 zipCrc32(const QByteArray& d) {
    static quint32 t[256]; static bool ok = false;
    if (!ok) {
        for (quint32 i = 0; i < 256; i++) {
            quint32 c = i;
            for (int j = 0; j < 8; j++)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            t[i] = c;
        }
        ok = true;
    }
    quint32 c = 0xFFFFFFFFu;
    for (unsigned char b : d) c = t[(c ^ b) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

struct ZipFile { QString name; QByteArray data; };

static QByteArray buildZip(const std::vector<ZipFile>& files) {
    QByteArray local;
    std::vector<quint32> offs, crcs;
    for (const auto& f : files) {
        offs.push_back((quint32)local.size());
        quint32 crc = zipCrc32(f.data); crcs.push_back(crc);
        quint32 sz = (quint32)f.data.size();
        QByteArray nm = f.name.toUtf8();
        zipU32(local, 0x04034B50u);
        zipU16(local, 20); zipU16(local, 0); zipU16(local, 0);
        zipU16(local, 0);  zipU16(local, 0);
        zipU32(local, crc); zipU32(local, sz); zipU32(local, sz);
        zipU16(local, (quint16)nm.size()); zipU16(local, 0);
        local.append(nm); local.append(f.data);
    }
    quint32 cdOff = (quint32)local.size();
    QByteArray cd;
    for (size_t i = 0; i < files.size(); i++) {
        QByteArray nm = files[i].name.toUtf8();
        quint32 sz = (quint32)files[i].data.size();
        zipU32(cd, 0x02014B50u);
        zipU16(cd, 20); zipU16(cd, 20);
        zipU16(cd, 0); zipU16(cd, 0); zipU16(cd, 0); zipU16(cd, 0);
        zipU32(cd, crcs[i]); zipU32(cd, sz); zipU32(cd, sz);
        zipU16(cd, (quint16)nm.size());
        zipU16(cd, 0); zipU16(cd, 0); zipU16(cd, 0); zipU16(cd, 0);
        zipU32(cd, 0); zipU32(cd, offs[i]);
        cd.append(nm);
    }
    QByteArray eocd;
    zipU32(eocd, 0x06054B50u);
    zipU16(eocd, 0); zipU16(eocd, 0);
    zipU16(eocd, (quint16)files.size()); zipU16(eocd, (quint16)files.size());
    zipU32(eocd, (quint32)cd.size()); zipU32(eocd, cdOff);
    zipU16(eocd, 0);
    return local + cd + eocd;
}

static QByteArray createMinimalDocx() {
    std::vector<ZipFile> f;
    f.push_back({"[Content_Types].xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
        "</Types>")});
    f.push_back({"_rels/.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/>"
        "</Relationships>")});
    f.push_back({"word/document.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:body><w:p><w:r><w:t></w:t></w:r></w:p><w:sectPr/></w:body>"
        "</w:document>")});
    f.push_back({"word/_rels/document.xml.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"/>")});
    return buildZip(f);
}

static QByteArray createMinimalXlsx() {
    std::vector<ZipFile> f;
    f.push_back({"[Content_Types].xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
        "<Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
        "</Types>")});
    f.push_back({"_rels/.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
        "</Relationships>")});
    f.push_back({"xl/workbook.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\""
        " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<sheets><sheet name=\"Sheet1\" sheetId=\"1\" r:id=\"rId1\"/></sheets>"
        "</workbook>")});
    f.push_back({"xl/_rels/workbook.xml.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>"
        "</Relationships>")});
    f.push_back({"xl/worksheets/sheet1.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<sheetData/></worksheet>")});
    return buildZip(f);
}

static QByteArray createMinimalPptx() {
    std::vector<ZipFile> f;
    f.push_back({"[Content_Types].xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/ppt/presentation.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml\"/>"
        "<Override PartName=\"/ppt/slides/slide1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>"
        "</Types>")});
    f.push_back({"_rels/.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"ppt/presentation.xml\"/>"
        "</Relationships>")});
    f.push_back({"ppt/presentation.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<p:presentation xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\""
        " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<p:sldIdLst><p:sldId id=\"256\" r:id=\"rId1\"/></p:sldIdLst>"
        "<p:sldSz cx=\"9144000\" cy=\"6858000\"/>"
        "</p:presentation>")});
    f.push_back({"ppt/_rels/presentation.xml.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" Target=\"slides/slide1.xml\"/>"
        "</Relationships>")});
    f.push_back({"ppt/slides/slide1.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<p:sld xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\""
        " xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\""
        " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<p:cSld><p:spTree>"
        "<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
        "<p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/>"
        "<a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"
        "</p:spTree></p:cSld>"
        "<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>"
        "</p:sld>")});
    f.push_back({"ppt/slides/_rels/slide1.xml.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"/>")});
    return buildZip(f);
}