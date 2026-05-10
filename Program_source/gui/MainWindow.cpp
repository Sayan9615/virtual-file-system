#include "MainWindow.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include "../filesystem/SharedFolder.h"
#include "../services/IFileManager.h"
#include "../services/IAuthService.h"
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

MainWindow::MainWindow(Folder* root, const std::string& username, IFileManager& fm, IAuthService& auth, QWidget* parent)
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

void MainWindow::saveSystemState() { logEvent("SYSTEM", "Datele au fost salvate automat in baza de date SQLite."); }
void MainWindow::loadSystemState() { logEvent("SYSTEM", "Sistemul a fost incarcat cu succes din SQLite."); }

void MainWindow::setupMenuBar() {
    QMenu* fileMenu      = menuBar()->addMenu("File");
    QAction* newFileAction   = fileMenu->addAction("Fisier nou");
    QAction* newFolderAction = fileMenu->addAction("Folder nou");
    fileMenu->addSeparator();
    QAction* editFileAction = fileMenu->addAction("Editeaza fisier (Enter)");
    editFileAction->setShortcut(QKeySequence(Qt::Key_Return));
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
    QAction* exportLogsAction = logMenu->addAction("Export log-uri");

    connect(exitAction,       &QAction::triggered, qApp, &QApplication::quit);
    connect(newFileAction,    &QAction::triggered, this, &MainWindow::onNewFile);
    connect(newFolderAction,  &QAction::triggered, this, &MainWindow::onNewFolder);
    connect(editFileAction,   &QAction::triggered, this, &MainWindow::onEditFile);
    connect(renameAction,     &QAction::triggered, this, &MainWindow::onRename);
    connect(deleteAction,     &QAction::triggered, this, &MainWindow::onDelete);
    connect(shareMenuAction,  &QAction::triggered, this, &MainWindow::onShareDialog);
    connect(manageGroupsAction, &QAction::triggered, this, &MainWindow::onManageGroups);
    connect(propertiesAction, &QAction::triggered, this, &MainWindow::onProperties);
    connect(sortNameAction,   &QAction::triggered, this, &MainWindow::onSortByName);
    connect(sortSizeAction,   &QAction::triggered, this, &MainWindow::onSortBySize);
    connect(sortDateAction,   &QAction::triggered, this, &MainWindow::onSortByDate);
    connect(exportLogsAction, &QAction::triggered, this, &MainWindow::onExportLogs);
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

    // ── SORTARE LA CLICK PE HEADER (VERSIUNEA FUNCTIONALA) ──
    m_treeView->header()->setSectionsClickable(true);
    m_treeView->header()->setSortIndicatorShown(true);

    connect(m_treeView->header(), &QHeaderView::sortIndicatorChanged, this, [this](int logicalIndex, Qt::SortOrder order) {
        if (logicalIndex == 0) {
            m_currentSort = (order == Qt::AscendingOrder) ? SortManager::SortCrit::NAME_ASC : SortManager::SortCrit::NAME_DESC;
            m_statusLabel->setText(order == Qt::AscendingOrder ? "Sortat dupa: Nume A→Z" : "Sortat dupa: Nume Z→A");
        } else if (logicalIndex == 2) {
            m_currentSort = (order == Qt::AscendingOrder) ? SortManager::SortCrit::SIZE_ASC : SortManager::SortCrit::SIZE_DESC;
            m_statusLabel->setText(order == Qt::AscendingOrder ? "Sortat dupa: Dimensiune (mic→mare)" : "Sortat dupa: Dimensiune (mare→mic)");
        } else if (logicalIndex == 3) {
            m_currentSort = (order == Qt::AscendingOrder) ? SortManager::SortCrit::DATE_ASC : SortManager::SortCrit::DATE_DESC;
            m_statusLabel->setText(order == Qt::AscendingOrder ? "Sortat dupa: Data (vechi→nou)" : "Sortat dupa: Data (nou→vechi)");
        }
        m_root->sortChildrenRecursive(m_currentSort);
        m_model->refresh();
    });

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
    m_statusLabel->setStyleSheet(
        "padding: 4px 10px;"
        "background-color: #2B2B2B;"
        "color: #B0B0B0;"
        "border-top: 1px solid #444444;"
        "font-size: 11px;");

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
    connect(m_searchResults, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        if (!item) return;
        QString path = item->data(Qt::UserRole).toString();
        auto entity = m_pathResolver->resolvePath(path.toStdString());
        if (entity) showEditDialog(entity.get());
    });

    connect(m_treeView, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
        if (!index.isValid()) return;
        auto* entity = m_model->entityFromIndex(index);
        if (entity && !entity->isFolder()) {
            showEditDialog(entity);
        }
    });

    // Deselectare / Creare in Root
    connect(m_treeView, &QTreeView::clicked, this, [this](const QModelIndex& index) {
        static QModelIndex lastClickedIndex;
        if (lastClickedIndex == index && m_treeView->selectionModel()->isSelected(index)) {
            m_treeView->clearSelection();
            m_treeView->setCurrentIndex(QModelIndex());
            lastClickedIndex = QModelIndex();
            m_addressBar->setText("/root");
            m_statusLabel->setText("Selectie anulata. Fisierele noi vor fi create in Root.");
        } else {
            lastClickedIndex = index;
        }
    });
}

// ── Permisiuni ───────────

bool MainWindow::checkWritePermission(FileSystemEntity* entity) {
    if (!entity) return false;

    if (entity->getId() <= 1) return true;

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

    if (entity->getId() <= 1) return true;

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
        if (folder) m_addressBar->setText(QString::fromStdString(folder->getAbsolutePath()));
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
    auto entity = m_pathResolver->resolvePath(path.toStdString());
    if (entity) showPreview(entity.get());
    logEvent("NAVIGATE", path.toStdString());
}

// ── Cautare ──────────────────────────────────────────

void MainWindow::onSearchTriggered() {
    QString query = m_searchBar->text().trimmed();
    if (query.isEmpty()) return;

    m_searchResults->clear();
    auto results = m_searchEngine.search(m_root, query.toStdString(), true, true);

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
    auto entity = m_pathResolver->resolvePath(path.toStdString());
    if (entity) showPreview(entity.get());
}

// Forward declarations for Office template builders (defined later in this file)
static QByteArray createMinimalDocx();
static QByteArray createMinimalXlsx();
static QByteArray createMinimalPptx();
static bool isBinaryOffice(const QString& name);

// ── Creare fisier FARA POPUP LOCATIE ─────────────────

void MainWindow::onNewFile() {
    auto index = m_treeView->currentIndex();
    Folder* parentFolder = m_root;

    if (index.isValid()) {
        auto* entity = m_model->entityFromIndex(index);
        if (entity) {
            if (entity->isFolder()) {
                parentFolder = dynamic_cast<Folder*>(entity);
            } else {
                auto parentIndex = m_model->parent(index);
                if (parentIndex.isValid()) {
                    parentFolder = dynamic_cast<Folder*>(m_model->entityFromIndex(parentIndex));
                }
            }
        }
    }

    if (!checkWritePermission(parentFolder)) return;

    int parentId = parentFolder->getId();
    if (parentId <= 0) parentId = 1;

    // ── Dialog cu selector de tip fisier ─────────────────────────
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
    const QStringList typeExts = {"txt", "docx", "xlsx", "pptx"};

    auto* nameEdit = new QLineEdit(&typeDialog);
    nameEdit->setText(QString("Document_%1.txt").arg(parentFolder->getChildCount() + 1));

    connect(typeCombo, &QComboBox::currentIndexChanged, &typeDialog, [&](int idx) {
        if (idx < 0 || idx >= typeExts.size()) return;
        QString ext  = typeExts[idx];
        QString base = nameEdit->text().trimmed();
        int dot = base.lastIndexOf('.');
        if (dot >= 0) base = base.left(dot);
        if (base.isEmpty()) base = QString("Document_%1").arg(parentFolder->getChildCount() + 1);
        nameEdit->setText(base + "." + ext);
    });

    formLayout->addRow("Tip fisier:", typeCombo);
    formLayout->addRow("Nume:",       nameEdit);
    dlgLayout->addLayout(formLayout);

    auto* btnRow   = new QHBoxLayout();
    auto* cancelB  = new QPushButton("Anuleaza", &typeDialog);
    auto* createB  = new QPushButton("Creeaza",  &typeDialog);
    createB->setDefault(true);
    btnRow->addStretch(); btnRow->addWidget(cancelB); btnRow->addWidget(createB);
    dlgLayout->addLayout(btnRow);
    connect(cancelB, &QPushButton::clicked, &typeDialog, &QDialog::reject);
    connect(createB, &QPushButton::clicked, &typeDialog, &QDialog::accept);

    if (typeDialog.exec() != QDialog::Accepted) return;

    QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) return;

    for (const auto& child : parentFolder->getChildren()) {
        if (QString::fromStdString(child->getName()).compare(name, Qt::CaseInsensitive) == 0) {
            QMessageBox::warning(this, "Eroare",
                QString("Exista deja un element cu numele '%1' in acest folder!").arg(name));
            return;
        }
    }

    // Continut initial: template valid pentru fisiere Office, spatiu pentru text
    std::string initialContent = " ";
    {
        QByteArray tpl;
        if (name.endsWith(".docx", Qt::CaseInsensitive))      tpl = createMinimalDocx();
        else if (name.endsWith(".xlsx", Qt::CaseInsensitive)) tpl = createMinimalXlsx();
        else if (name.endsWith(".pptx", Qt::CaseInsensitive)) tpl = createMinimalPptx();
        if (!tpl.isEmpty()) initialContent = tpl.toBase64().toStdString();
    }

    try {
        bool saved = m_fm.createTextFile(name.toStdString(), m_currentUser, "users", initialContent, parentId);
        if (!saved) { QMessageBox::warning(this, "Eroare", "Eroare la salvare in baza de date."); return; }

        int newId = m_fm.getEntityId(name.toStdString(), parentId);

        auto newFile = std::make_shared<TextFile>(name.toStdString(), m_currentUser, "users", initialContent);
        if (newId != -1) newFile->setId(newId);

        parentFolder->addChild(newFile);
        m_model->refresh();

        if (index.isValid() && m_model->entityFromIndex(index) && m_model->entityFromIndex(index)->isFolder()) {
            m_treeView->expand(index);
        }

        logEvent("CREATE_FILE", name.toStdString());
        m_statusLabel->setText("Fisier creat: " + name);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

// ── Creare folder FARA POPUP LOCATIE ─────────────────

void MainWindow::onNewFolder() {
    auto index = m_treeView->currentIndex();
    Folder* parentFolder = m_root;

    if (index.isValid()) {
        auto* entity = m_model->entityFromIndex(index);
        if (entity) {
            if (entity->isFolder()) {
                parentFolder = dynamic_cast<Folder*>(entity);
            } else {
                auto parentIndex = m_model->parent(index);
                if (parentIndex.isValid()) {
                    parentFolder = dynamic_cast<Folder*>(m_model->entityFromIndex(parentIndex));
                }
            }
        }
    }

    if (!checkWritePermission(parentFolder)) return;

    int parentId = parentFolder->getId();
    if (parentId <= 0) parentId = 1;

    bool ok;
    QString name = QInputDialog::getText(this, "Folder nou", "Numele folderului:",
                                         QLineEdit::Normal, QString("Folder_%1").arg(parentFolder->getChildCount() + 1), &ok);

    if (!ok || name.trimmed().isEmpty()) return;
    name = name.trimmed();

    if (parentFolder->hasChild(name.toStdString())) {
        QMessageBox::warning(this, "Eroare", QString("Exista deja un element cu numele '%1' in acest folder!").arg(name));
        return;
    }

    try {
        bool saved = m_fm.createFolder(name.toStdString(), m_currentUser, "users", parentId);
        if (!saved) { QMessageBox::warning(this, "Eroare", "Eroare la salvare in baza de date."); return; }

        int newId = m_fm.getEntityId(name.toStdString(), parentId);

        auto newFolder = std::make_shared<Folder>(name.toStdString(), m_currentUser, "users", parentFolder);
        if (newId != -1) newFolder->setId(newId);

        parentFolder->addChild(newFolder);
        m_model->refresh();

        if (index.isValid() && m_model->entityFromIndex(index) && m_model->entityFromIndex(index)->isFolder()) {
            m_treeView->expand(index);
        }

        logEvent("CREATE_FOLDER", name.toStdString());
        m_statusLabel->setText("Folder creat: " + name);
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

// ── Editare Text ─────────────────────────────────────

void MainWindow::onEditFile() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    auto* entity = m_model->entityFromIndex(index);
    if (entity && !entity->isFolder()) {
        showEditDialog(entity);
    }
}

void MainWindow::showEditDialog(FileSystemEntity* entity) {
    if (!entity || entity->isFolder()) return;
    std::string name = entity->getName();

    if (name.find(".docx") != std::string::npos ||
        name.find(".pptx") != std::string::npos ||
        name.find(".xlsx") != std::string::npos ||
        name.find(".pdf") != std::string::npos) {
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

    auto* btnLayout = new QHBoxLayout();
    auto* cancelBtn = new QPushButton("Anuleaza", &dialog);
    auto* saveBtn   = new QPushButton("💾 Salveaza", &dialog);
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    layout->addLayout(btnLayout);

    auto* saveShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), &dialog);
    connect(saveShortcut, &QShortcut::activated, saveBtn, &QPushButton::click);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    connect(saveBtn, &QPushButton::clicked, &dialog, [&]() {
        QString newContent = editor->toPlainText();
        try {
            if (m_fm.updateTextFile(entity->getId(), newContent.toStdString(), m_currentUser)) {
                tf->write(newContent.toStdString());
                showPreview(entity);
                updateStatusBar(entity);
                m_model->refresh();
                logEvent("EDIT_FILE", entity->getName());
                dialog.accept();
                m_statusLabel->setText(QString("Salvat: %1").arg(QString::fromStdString(entity->getName())));
            } else {
                QMessageBox::warning(&dialog, "Eroare", "Eroare la salvare in baza de date.");
            }
        } catch (const std::exception& e) { QMessageBox::warning(&dialog, "Eroare", e.what()); }
    });

    dialog.exec();
}

// ── Helper: detecteaza fisiere binare Office ──────────

static bool isBinaryOffice(const QString& name) {
    return name.endsWith(".docx", Qt::CaseInsensitive) ||
           name.endsWith(".pptx", Qt::CaseInsensitive) ||
           name.endsWith(".xlsx", Qt::CaseInsensitive);
}

// ── ZIP builder + minimal Office templates ───────────────────────

static void zipU16(QByteArray& ba, quint16 v) { ba.append(char(v&0xFF)); ba.append(char((v>>8)&0xFF)); }
static void zipU32(QByteArray& ba, quint32 v) { ba.append(char(v&0xFF)); ba.append(char((v>>8)&0xFF)); ba.append(char((v>>16)&0xFF)); ba.append(char((v>>24)&0xFF)); }

static quint32 zipCrc32(const QByteArray& d) {
    static quint32 t[256]; static bool ok=false;
    if (!ok) { for (quint32 i=0;i<256;i++){quint32 c=i;for(int j=0;j<8;j++)c=(c&1)?(0xEDB88320u^(c>>1)):(c>>1);t[i]=c;} ok=true; }
    quint32 c=0xFFFFFFFFu;
    for (unsigned char b : d) c=t[(c^b)&0xFF]^(c>>8);
    return c^0xFFFFFFFFu;
}

struct ZipFile { QString name; QByteArray data; };

static QByteArray buildZip(const std::vector<ZipFile>& files) {
    QByteArray local; std::vector<quint32> offs, crcs;
    for (const auto& f : files) {
        offs.push_back((quint32)local.size());
        quint32 crc=zipCrc32(f.data); crcs.push_back(crc);
        quint32 sz=(quint32)f.data.size(); QByteArray nm=f.name.toUtf8();
        zipU32(local,0x04034B50u); zipU16(local,20); zipU16(local,0); zipU16(local,0);
        zipU16(local,0); zipU16(local,0);
        zipU32(local,crc); zipU32(local,sz); zipU32(local,sz);
        zipU16(local,(quint16)nm.size()); zipU16(local,0);
        local.append(nm); local.append(f.data);
    }
    quint32 cdOff=(quint32)local.size(); QByteArray cd;
    for (size_t i=0;i<files.size();i++) {
        QByteArray nm=files[i].name.toUtf8(); quint32 sz=(quint32)files[i].data.size();
        zipU32(cd,0x02014B50u); zipU16(cd,20); zipU16(cd,20); zipU16(cd,0); zipU16(cd,0);
        zipU16(cd,0); zipU16(cd,0);
        zipU32(cd,crcs[i]); zipU32(cd,sz); zipU32(cd,sz);
        zipU16(cd,(quint16)nm.size()); zipU16(cd,0); zipU16(cd,0);
        zipU16(cd,0); zipU16(cd,0); zipU32(cd,0); zipU32(cd,offs[i]);
        cd.append(nm);
    }
    QByteArray eocd;
    zipU32(eocd,0x06054B50u); zipU16(eocd,0); zipU16(eocd,0);
    zipU16(eocd,(quint16)files.size()); zipU16(eocd,(quint16)files.size());
    zipU32(eocd,(quint32)cd.size()); zipU32(eocd,cdOff); zipU16(eocd,0);
    return local+cd+eocd;
}

static QByteArray createMinimalDocx() {
    std::vector<ZipFile> f;
    f.push_back({"[Content_Types].xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/word/document.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml\"/>"
        "</Types>")});
    f.push_back({"_rels/.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"word/document.xml\"/>"
        "</Relationships>")});
    f.push_back({"word/document.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\">"
        "<w:body><w:p><w:r><w:t></w:t></w:r></w:p><w:sectPr/></w:body>"
        "</w:document>")});
    f.push_back({"word/_rels/document.xml.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\"/>")});
    return buildZip(f);
}

static QByteArray createMinimalXlsx() {
    std::vector<ZipFile> f;
    f.push_back({"[Content_Types].xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/xl/workbook.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml\"/>"
        "<Override PartName=\"/xl/worksheets/sheet1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>"
        "<Override PartName=\"/xl/styles.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml\"/>"
        "</Types>")});
    f.push_back({"_rels/.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"xl/workbook.xml\"/>"
        "</Relationships>")});
    f.push_back({"xl/workbook.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<workbook xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\""
        " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<sheets><sheet name=\"Sheet1\" sheetId=\"1\" r:id=\"rId1\"/></sheets>"
        "</workbook>")});
    f.push_back({"xl/_rels/workbook.xml.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet1.xml\"/>"
        "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/styles\" Target=\"styles.xml\"/>"
        "</Relationships>")});
    f.push_back({"xl/worksheets/sheet1.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<worksheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\"><sheetData/></worksheet>")});
    f.push_back({"xl/styles.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<styleSheet xmlns=\"http://schemas.openxmlformats.org/spreadsheetml/2006/main\">"
        "<fonts count=\"1\"><font><sz val=\"11\"/><name val=\"Calibri\"/></font></fonts>"
        "<fills count=\"2\"><fill><patternFill patternType=\"none\"/></fill><fill><patternFill patternType=\"gray125\"/></fill></fills>"
        "<borders count=\"1\"><border><left/><right/><top/><bottom/><diagonal/></border></borders>"
        "<cellStyleXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\"/></cellStyleXfs>"
        "<cellXfs count=\"1\"><xf numFmtId=\"0\" fontId=\"0\" fillId=\"0\" borderId=\"0\" xfId=\"0\"/></cellXfs>"
        "</styleSheet>")});
    return buildZip(f);
}

static QByteArray createMinimalPptx() {
    std::vector<ZipFile> f;
    f.push_back({"[Content_Types].xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
        "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
        "<Default Extension=\"xml\" ContentType=\"application/xml\"/>"
        "<Override PartName=\"/ppt/presentation.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml\"/>"
        "<Override PartName=\"/ppt/slideMasters/slideMaster1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml\"/>"
        "<Override PartName=\"/ppt/slideLayouts/slideLayout1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml\"/>"
        "<Override PartName=\"/ppt/slides/slide1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>"
        "</Types>")});
    f.push_back({"_rels/.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument\" Target=\"ppt/presentation.xml\"/>"
        "</Relationships>")});
    f.push_back({"ppt/presentation.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<p:presentation xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\""
        " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<p:sldMasterIdLst><p:sldMasterId id=\"2147483648\" r:id=\"rId1\"/></p:sldMasterIdLst>"
        "<p:sldIdLst><p:sldId id=\"256\" r:id=\"rId2\"/></p:sldIdLst>"
        "<p:sldSz cx=\"9144000\" cy=\"6858000\"/><p:notesSz cx=\"6858000\" cy=\"9144000\"/>"
        "</p:presentation>")});
    f.push_back({"ppt/_rels/presentation.xml.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\" Target=\"slideMasters/slideMaster1.xml\"/>"
        "<Relationship Id=\"rId2\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" Target=\"slides/slide1.xml\"/>"
        "</Relationships>")});
    f.push_back({"ppt/slideMasters/slideMaster1.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<p:sldMaster xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\""
        " xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\""
        " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<p:cSld><p:bg><p:bgPr><a:solidFill><a:srgbClr val=\"FFFFFF\"/></a:solidFill></p:bgPr></p:bg>"
        "<p:spTree><p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
        "<p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/>"
        "<a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"
        "</p:spTree></p:cSld>"
        "<p:clrMap bg1=\"lt1\" tx1=\"dk1\" bg2=\"lt2\" tx2=\"dk2\" accent1=\"accent1\" accent2=\"accent2\" accent3=\"accent3\" accent4=\"accent4\" accent5=\"accent5\" accent6=\"accent6\" hlink=\"hlink\" folHlink=\"folHlink\"/>"
        "<p:sldLayoutIdLst><p:sldLayoutId id=\"2147483649\" r:id=\"rId1\"/></p:sldLayoutIdLst>"
        "<p:txStyles>"
        "<p:titleStyle><a:lvl1pPr><a:defRPr lang=\"en-US\"/></a:lvl1pPr></p:titleStyle>"
        "<p:bodyStyle><a:lvl1pPr><a:defRPr lang=\"en-US\"/></a:lvl1pPr></p:bodyStyle>"
        "<p:otherStyle><a:lvl1pPr><a:defRPr lang=\"en-US\"/></a:lvl1pPr></p:otherStyle>"
        "</p:txStyles></p:sldMaster>")});
    f.push_back({"ppt/slideMasters/_rels/slideMaster1.xml.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/>"
        "</Relationships>")});
    f.push_back({"ppt/slideLayouts/slideLayout1.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<p:sldLayout xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\""
        " xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\""
        " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\""
        " type=\"blank\" preserve=\"1\">"
        "<p:cSld name=\"Blank\"><p:spTree>"
        "<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
        "<p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/>"
        "<a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"
        "</p:spTree></p:cSld>"
        "<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sldLayout>")});
    f.push_back({"ppt/slideLayouts/_rels/slideLayout1.xml.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster\" Target=\"../slideMasters/slideMaster1.xml\"/>"
        "</Relationships>")});
    f.push_back({"ppt/slides/slide1.xml", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<p:sld xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\""
        " xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\""
        " xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
        "<p:cSld><p:spTree>"
        "<p:nvGrpSpPr><p:cNvPr id=\"1\" name=\"\"/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>"
        "<p:grpSpPr><a:xfrm><a:off x=\"0\" y=\"0\"/><a:ext cx=\"0\" cy=\"0\"/>"
        "<a:chOff x=\"0\" y=\"0\"/><a:chExt cx=\"0\" cy=\"0\"/></a:xfrm></p:grpSpPr>"
        "</p:spTree></p:cSld>"
        "<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr></p:sld>")});
    f.push_back({"ppt/slides/_rels/slide1.xml.rels", QByteArray(
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
        "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
        "<Relationship Id=\"rId1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout\" Target=\"../slideLayouts/slideLayout1.xml\"/>"
        "</Relationships>")});
    return buildZip(f);
}

// ── Deschidere si Sincronizare Office (.docx/.pptx/.xlsx) ────────
// Continutul binar e stocat in DB ca base64.
// La deschidere: decodam base64 -> scriem bytes reali in fisier temp.
// La sync-back:  citim bytes reali -> encodam base64 -> salvam in DB.

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

        QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/" + fileName;
        QFile realFile(tempPath);

        if (!realFile.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, "Eroare", "Nu s-a putut crea fisierul temporar!");
            return;
        }

        std::string stored = tf->read();
        if (binary) {
            QByteArray raw = QByteArray::fromBase64(QString::fromStdString(stored).trimmed().toUtf8());
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
        bool canWrite = (entity->getOwnerUser() == m_currentUser) ||
                        m_fm.checkPermission(idToCheck, m_currentUser, "write");

        if (canWrite) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Sincronizare");
            msgBox.setText(QString("Documentul '%1' a fost deschis.\n\n"
                                   "Editeaza, salveaza (Ctrl+S),\n"
                                   "inchide aplicatia Office, apoi apasa OK.").arg(fileName));
            msgBox.setIcon(QMessageBox::Information);
            msgBox.exec();

            if (realFile.open(QIODevice::ReadOnly)) {
                QByteArray newData = realFile.readAll();
                realFile.close();

                std::string toStore;
                if (binary) {
                    // Encodam bytes reali -> base64 pentru stocare in DB
                    toStore = newData.toBase64().toStdString();
                } else {
                    toStore = std::string(newData.constData(), newData.size());
                }

                if (m_fm.updateTextFile(entity->getId(), toStore, m_currentUser)) {
                    tf->write(toStore);
                    m_model->refresh();
                    showPreview(entity);
                    m_statusLabel->setText("Sincronizare reusita: " + fileName);
                    logEvent("UPDATE_FROM_EXTERNAL", entity->getName());
                } else {
                    QMessageBox::warning(this, "Eroare DB", "Modificarile nu au putut fi salvate in sistem.");
                }
            }
        } else {
            m_statusLabel->setText("Deschis in modul Read-Only.");
        }
    } catch (const std::exception& e) {
        QMessageBox::warning(this, "Eroare", QString::fromStdString(e.what()));
    }
}

// ── Redenumire / Stergere ────────────────────────────

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

void MainWindow::onDelete() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;

    auto* entity = m_model->entityFromIndex(index);
    if (!entity) return;

    if (!checkWritePermission(entity)) return;

    auto reply = QMessageBox::question(this, "Confirmare stergere",
                                       QString("Esti sigur ca vrei ca stergi '%1'?").arg(QString::fromStdString(entity->getName())),
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

// ── Context Menu (Click Dreapta) ─────────────────────

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
                openExternalAction = contextMenu.addAction("🖥 Deschide extern (Word/Windows)");
                editAction = contextMenu.addAction("📝 Editeaza Text in aplicatie");
                contextMenu.addSeparator();
            }

            renameAction = contextMenu.addAction("✏ Redenumire (F2)");

            deleteAction = contextMenu.addAction("🗑 Sterge");
            int idToCheck = entity->getId() <= 0 ? 1 : entity->getId();
            bool canWrite = (entity->getOwnerUser() == m_currentUser) || m_fm.checkPermission(idToCheck, m_currentUser, "write");
            if (!canWrite) {
                deleteAction->setEnabled(false);
                deleteAction->setToolTip("Nu aveti permisiunea de scriere!");
            }

            contextMenu.addSeparator();

            shareAction = contextMenu.addAction("🔗 Partajare (Share)...");
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
    if (editAction && selected == editAction) { showEditDialog(entity); return; }
    if (renameAction && selected == renameAction) { onRename(); return; }
    if (deleteAction && selected == deleteAction) { onDelete(); return; }
    if (openExternalAction && selected == openExternalAction) { onOpenExternal(); return; }
    if (shareAction && selected == shareAction) { showShareDialog(entity); return; }
    if (propertiesAction && selected == propertiesAction) { showPropertiesDialog(entity); return; }
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

void MainWindow::onShareDialog() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    auto* entity = m_model->entityFromIndex(index);
    if (entity) showShareDialog(entity);
}

void MainWindow::showShareDialog(FileSystemEntity* entity) {
    if (!entity) return;

    if (entity->getOwnerUser() != m_currentUser) {
        QMessageBox::warning(this, "Acces refuzat", "Doar proprietarul poate partaja aceasta resursa!");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(QString("Partajare — %1").arg(QString::fromStdString(entity->getName())));
    dialog.setFixedSize(400, 500);

    auto* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* titleLabel = new QLabel(QString("<b>Partajeaza '%1' cu:</b>").arg(QString::fromStdString(entity->getName())), &dialog);
    layout->addWidget(titleLabel);

    auto* listWidget = new QListWidget(&dialog);
    listWidget->setSelectionMode(QAbstractItemView::NoSelection);

    // 1. Incarcam UTILIZATORII
    auto allUsers = m_auth.getAllUsers();
    std::vector<std::string> alreadyShared = m_fm.getSharedWithList(entity->getId());

    for (const auto& [id, uname] : allUsers) {
        if (uname == m_currentUser) continue;

        auto* item = new QListWidgetItem(QString("👤 ") + QString::fromStdString(uname), listWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

        bool isShared = std::find(alreadyShared.begin(), alreadyShared.end(), uname) != alreadyShared.end();
        item->setCheckState(isShared ? Qt::Checked : Qt::Unchecked);

        if (isShared) item->setForeground(QColor(0, 120, 215));
        item->setData(Qt::UserRole, "USER");
        item->setData(Qt::UserRole + 1, QString::fromStdString(uname));
    }

    // 2. Incarcam GRUPURILE (modificat pentru a retine salvarea)
    auto allGroups = m_auth.getAllGroups();
    for (const auto& gName : allGroups) {
        auto* item = new QListWidgetItem(QString("👥 [Grup] ") + QString::fromStdString(gName), listWidget);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

        auto members = m_auth.getUsersInGroup(gName);
        bool allShared = (!members.empty()); // Verificam daca toti membrii au primit deja share

        for (const auto& member : members) {
            if (member != m_currentUser && std::find(alreadyShared.begin(), alreadyShared.end(), member) == alreadyShared.end()) {
                allShared = false;
                break;
            }
        }

        // Daca toti membrii grupului au acces, grupul va aparea BIFAT (Salvat)
        item->setCheckState(allShared ? Qt::Checked : Qt::Unchecked);
        item->setForeground(QColor(0, 150, 0));
        item->setData(Qt::UserRole, "GROUP");
        item->setData(Qt::UserRole + 1, QString::fromStdString(gName));
    }

    if (listWidget->count() == 0) listWidget->addItem("Nu exista alti utilizatori inregistrati.");
    layout->addWidget(listWidget);

    auto* permGroup  = new QGroupBox("Nivel acces acordat", &dialog);
    auto* permLayout = new QVBoxLayout(permGroup);
    auto* canReadCheck  = new QCheckBox("Poate Citi",      permGroup);
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

            QString type = item->data(Qt::UserRole).toString();
            QString name = item->data(Qt::UserRole + 1).toString();
            bool isChecked = (item->checkState() == Qt::Checked);

            if (type == "USER") {
                std::string uname = name.toStdString();
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
            else if (type == "GROUP") {
                auto members = m_auth.getUsersInGroup(name.toStdString());
                for (const auto& member : members) {
                    if (member != m_currentUser) {
                        bool wasShared = std::find(alreadyShared.begin(), alreadyShared.end(), member) != alreadyShared.end();

                        // Daca bifam grupul si nu avea share
                        if (isChecked && !wasShared) {
                            try {
                                m_fm.shareEntity(entity->getId(), member);
                                m_fm.addUserToGroup(entity->getId(), member);
                                m_fm.setPermission(entity->getId(), member, canReadCheck->isChecked(), canWriteCheck->isChecked());
                                sharedCount++;
                            } catch (...) {}
                        }
                        // Daca DEBIFAM grupul si inainte avea share
                        else if (!isChecked && wasShared) {
                            try {
                                m_fm.revokeShare(entity->getId(), member);
                                m_fm.removeUserFromGroup(entity->getId(), member);
                            } catch (...) {}
                        }
                    }
                }
            }
        }
        logEvent("SHARE", entity->getName() + " -> aplicat pentru useri noi");
        dialog.accept();
    });

    if (dialog.exec() == QDialog::Accepted) {
        m_model->refresh();
        QMessageBox::information(this, "Partajare", "Setarile de partajare au fost aplicate si salvate!");
    }
}

// ── Dialog proprietati ───────────────────────────────

void MainWindow::onProperties() {
    auto index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    auto* entity = m_model->entityFromIndex(index);
    if (entity) showPropertiesDialog(entity);
}

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

// ── Preview & Utilitare ──────────────────────────────

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
            QString name = QString::fromStdString(entity->getName());
            QString docType = "Fisier text/Document";

            // Detectam extensia pentru a afisa tipul corect
            if (name.endsWith(".docx", Qt::CaseInsensitive)) docType = "Document Microsoft Word (.docx)";
            else if (name.endsWith(".pptx", Qt::CaseInsensitive)) docType = "Prezentare PowerPoint (.pptx)";
            else if (name.endsWith(".xlsx", Qt::CaseInsensitive)) docType = "Registru Excel (.xlsx)";
            else if (name.endsWith(".pdf", Qt::CaseInsensitive)) docType = "Document PDF (.pdf)";

            info += "<b>Tip:</b> " + docType + "<br><hr>";
            if (isBinaryOffice(name)) {
                QByteArray raw = QByteArray::fromBase64(QByteArray::fromStdString(tf->read()));
                info += QString("<i>Fisier binar — %1 bytes stocati.</i>").arg(raw.size());
            } else {
                info += "<pre>" + QString::fromStdString(tf->read()).toHtmlEscaped() + "</pre>";
            }

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

void MainWindow::onShowLogs() {
    if (m_logConsole) m_logConsole->show();
}

void MainWindow::onExportLogs() {
    if (m_logger) {
        m_logger->exportLogs("export_logs.txt");
        QMessageBox::information(this, "Export", "Log-urile au fost exportate in 'export_logs.txt'!");
    }
}

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