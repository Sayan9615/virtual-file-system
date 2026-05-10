#pragma once
#include <QMainWindow>
#include <QTreeView>
#include <QTextEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QSplitter>
#include <QPushButton>
#include <QHeaderView>
#include <QCloseEvent>
#include <QMenu>
#include <QShortcut>

#include "../filesystem/Folder.h"
#include "../services/IFileManager.h"
#include "../services/IAuthService.h"
#include "../services/SearchEngine.h"
#include "FileSystemModel.h"
#include "PathResolver.h"
#include "../logger/TimestampedLogger.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(Folder* root, const std::string& username, IFileManager& fm, IAuthService& auth, QWidget* parent = nullptr);
    ~MainWindow() = default;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onNewFile();
    void onNewFolder();
    void onRename();
    void onDelete();
    void onEditFile();
    void onOpenExternal(FileSystemEntity* entityOverride = nullptr);
    void onShareDialog();
    void onManageGroups();
    void onProperties();
    void onSortByName();
    void onSortBySize();
    void onSortByDate();
    void onShowLogs();
    void onExportLogs();
    void onSearchTriggered();
    void onAddressBarEntered();
    void onItemSelected(const QModelIndex& index);
    void onSearchResultClicked(QListWidgetItem* item);
    void onContextMenu(const QPoint& pos);

private:
    void setupMenuBar();
    void setupUI();
    void showPreview(FileSystemEntity* entity);
    void updateStatusBar(FileSystemEntity* entity);
    void logEvent(const std::string& action, const std::string& path);
    bool checkWritePermission(FileSystemEntity* entity);
    bool checkReadPermission(FileSystemEntity* entity);
    void showShareDialog(FileSystemEntity* entity);
    void showPropertiesDialog(FileSystemEntity* entity);
    void showEditDialog(FileSystemEntity* entity);
    void saveSystemState();
    void loadSystemState();

    // Membrii
    Folder* m_root;
    std::string m_currentUser;
    IFileManager& m_fm;
    IAuthService& m_auth;
    FileSystemModel* m_model;
    PathResolver* m_pathResolver;
    TimestampedLogger* m_logger;

    // Elemente UI
    QTreeView* m_treeView;
    QTextEdit* m_previewPane;
    QLineEdit* m_searchBar;
    QLineEdit* m_addressBar;
    QListWidget* m_searchResults;
    QTextEdit* m_logConsole;
    QLabel* m_statusLabel;
    QSplitter* m_mainSplitter;
    SortManager::SortCrit m_currentSort;
    SearchEngine m_searchEngine;
};