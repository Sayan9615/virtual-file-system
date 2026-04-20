#include "MainWindow.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QHeaderView>
#include <QPushButton>
#include <QStatusBar>

MainWindow::MainWindow(Folder* root, QWidget* parent)
    : QMainWindow(parent), m_root(root) {
    m_model = new FileSystemModel(root, this);
    setupUI();
    setupMenuBar();
    setWindowTitle("ATMosFILE");
    resize(1100, 700);
}

void MainWindow::setupUI() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainLayout = new QVBoxLayout(central);

    // ── Search bar ────────────────────────────────────────────────
    auto* searchLayout = new QHBoxLayout();
    m_searchBar = new QLineEdit();
    m_searchBar->setPlaceholderText("Cauta fisiere sau continut... (ex: *.txt, hello world)");
    m_searchBar->setMinimumHeight(32);

    auto* searchBtn = new QPushButton("Cauta");
    searchBtn->setMinimumHeight(32);

    searchLayout->addWidget(m_searchBar);
    searchLayout->addWidget(searchBtn);
    mainLayout->addLayout(searchLayout);

    // ── Splitter principal ────────────────────────────────────────
    m_mainSplitter = new QSplitter(Qt::Horizontal);

    // Stanga: tree view
    m_treeView = new QTreeView();
    m_treeView->setModel(m_model);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(20);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeView->setMinimumWidth(300);

    // Dreapta: splitter vertical
    auto* rightSplitter = new QSplitter(Qt::Vertical);

    m_previewPane = new QTextEdit();
    m_previewPane->setReadOnly(true);
    m_previewPane->setPlaceholderText("Selecteaza un fisier pentru preview...");

    m_searchResults = new QListWidget();
    m_searchResults->setVisible(false);

    rightSplitter->addWidget(m_previewPane);
    rightSplitter->addWidget(m_searchResults);
    rightSplitter->setStretchFactor(0, 3);
    rightSplitter->setStretchFactor(1, 1);

    m_mainSplitter->addWidget(m_treeView);
    m_mainSplitter->addWidget(rightSplitter);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 2);

    mainLayout->addWidget(m_mainSplitter);

    // ── Status bar ────────────────────────────────────────────────
    m_statusBar = new QLabel("Gata.");
    statusBar()->addWidget(m_statusBar);

    // ── Conexiuni ─────────────────────────────────────────────────
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &MainWindow::onItemSelected);
    connect(searchBtn, &QPushButton::clicked,
            this, &MainWindow::onSearchTriggered);
    connect(m_searchBar, &QLineEdit::returnPressed,
            this, &MainWindow::onSearchTriggered);
    connect(m_searchResults, &QListWidget::itemClicked,
            this, &MainWindow::onSearchResultClicked);
}

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("Fisier");
    fileMenu->addAction("Nou folder", []{});
    fileMenu->addAction("Nou fisier", []{});
    fileMenu->addSeparator();
    fileMenu->addAction("Iesire", this, &QWidget::close);

    auto* viewMenu = menuBar()->addMenu("Vizualizare");
    viewMenu->addAction("Expandeaza tot",  [this]{ m_treeView->expandAll(); });
    viewMenu->addAction("Restrange tot",   [this]{ m_treeView->collapseAll(); });
}

void MainWindow::onItemSelected(const QModelIndex& index) {
    auto* entity = m_model->entityFromIndex(index);
    if (entity) showPreview(entity);
}

void MainWindow::showPreview(FileSystemEntity* entity) {
    if (entity->isFolder()) {
        auto* folder = static_cast<Folder*>(entity);
        QString info = QString("[ FOLDER ] %1\n\nCale:   %2\nOwner:  %3\nFisiere: %4")
            .arg(QString::fromStdString(folder->getName()))
            .arg(QString::fromStdString(folder->getAbsolutePath()))
            .arg(QString::fromStdString(folder->getOwnerUser()))
            .arg(folder->getChildCount());
        m_previewPane->setPlainText(info);
    } else {
        if (auto* tf = dynamic_cast<TextFile*>(entity)) {
            m_previewPane->setPlainText(QString::fromStdString(tf->read()));
        } else if (auto* bf = dynamic_cast<BinaryFile*>(entity)) {
            auto data = bf->getData();
            QString hex;
            for (int i = 0; i < (int)std::min(data.size(), (size_t)256); i++)
                hex += QString("%1 ").arg((unsigned char)data[i], 2, 16, QChar('0'));
            m_previewPane->setPlainText("[BINAR] " + hex + (data.size() > 256 ? "..." : ""));
        }
    }
    m_statusBar->setText(QString::fromStdString(entity->getName()) + " selectat");
}

void MainWindow::onSearchTriggered() {
    QString query = m_searchBar->text().trimmed();
    if (query.isEmpty()) return;

    auto results = m_searchEngine.search(m_root, query.toStdString());

    m_searchResults->clear();
    m_searchResults->setVisible(true);

    if (results.empty()) {
        m_searchResults->addItem("Niciun rezultat pentru: " + query);
        m_statusBar->setText("0 rezultate.");
        return;
    }

    for (const auto& r : results) {
        QString label = QString::fromStdString(r.absolutePath);
        if (!r.matchingLines.empty())
            label += QString(" (%1 match-uri)").arg(static_cast<int>(r.matchingLines.size()));

        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, QString::fromStdString(r.absolutePath));
        m_searchResults->addItem(item);
    }

    m_statusBar->setText(
        QString("%1 rezultate pentru \"%2\"")
            .arg(static_cast<int>(results.size()))
            .arg(query)
    );
}

void MainWindow::onSearchResultClicked(QListWidgetItem* item) {
    QString path = item->data(Qt::UserRole).toString();
    m_statusBar->setText("Selectat: " + path);
}