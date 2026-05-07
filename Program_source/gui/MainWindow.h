#pragma once
#include <QCloseEvent>
#include <QMainWindow>
#include <QTreeView>
#include <QTextEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QKeySequence>
#include <memory>
#include "../filesystem/Folder.h"
#include "../filesystem/SharedFolder.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include "../services/SearchEngine.h"
#include "../services/PathResolver.h"
#include "../services/SortManager.h"
#include "../logger/TimestampedLogger.h"
#include "FileSystemModel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

protected:
    void closeEvent(QCloseEvent *event) override;

public:
    explicit MainWindow(Folder* root,
                        const std::string& username,
                        QWidget* parent = nullptr);

private slots:
    void onOpenExternal();


    void onItemSelected(const QModelIndex& index);
    void onAddressBarEntered();
    void onSearchTriggered();
    void onSearchResultClicked(QListWidgetItem* item);
    void onNewFile();
    void onNewFolder();
    void onRename();
    void onDelete();

    void onSortByName();
    void onSortBySize();
    void onSortByDate();
    void onExportLogs();
    void onShowLogs();
    void onContextMenu(const QPoint& pos);

private:
    void saveSystemState();
    void loadSystemState();
    void setupUI();
    void setupMenuBar();
    void showPreview(FileSystemEntity* entity);
    void updateStatusBar(FileSystemEntity* entity);
    void logEvent(const std::string& action, const std::string& path = "");

    // ── Date ─────────────────────────────────────────
    Folder*      m_root;
    std::string  m_currentUser;

    // ── Servicii ─────────────────────────────────────
    FileSystemModel*          m_model;
    SearchEngine              m_searchEngine;
    PathResolver*             m_pathResolver;
    TimestampedLogger*        m_logger;
    SortManager::SortCrit m_currentSort;

    // ── UI ───────────────────────────────────────────
    QTreeView*   m_treeView;
    QTextEdit*   m_previewPane;
    QLineEdit*   m_searchBar;
    QLineEdit*   m_addressBar;
    QListWidget* m_searchResults;
    QSplitter*   m_mainSplitter;
    QLabel*      m_statusLabel;
    QTextEdit*   m_logConsole;
};