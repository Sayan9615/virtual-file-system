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
#include <QDialog>
#include <QGroupBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <memory>
#include "../filesystem/Folder.h"
#include "../filesystem/SharedFolder.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include "../interfaces/IShareable.h"
#include "../services/SearchEngine.h"
#include "../services/PathResolver.h"
#include "../services/SortManager.h"
#include "../services/FileManager.h"
#include "../services/AuthService.h"
#include "../logger/TimestampedLogger.h"
#include "FileSystemModel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

protected:
    void closeEvent(QCloseEvent* event) override;

public:
    explicit MainWindow(Folder* root,
                        const std::string& username,
                        FileManager& fm,
                        AuthService& auth,
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
    void onProperties();
    void onShareDialog();
    void onManageGroups(); // NOU
    void onSortByName();
    void onSortBySize();
    void onSortByDate();
    void onExportLogs();
    void onShowLogs();
    void onContextMenu(const QPoint& pos);

private:
    void setupUI();
    void setupMenuBar();

    void saveSystemState();
    void loadSystemState();

    void showPreview(FileSystemEntity* entity);
    void updateStatusBar(FileSystemEntity* entity);
    void logEvent(const std::string& action,
                  const std::string& path = "");

    bool checkWritePermission(FileSystemEntity* entity);
    bool checkReadPermission(FileSystemEntity* entity);

    void showShareDialog(FileSystemEntity* entity);
    void showPropertiesDialog(FileSystemEntity* entity);

    Folder*      m_root;
    std::string  m_currentUser;
    FileManager& m_fm;
    AuthService& m_auth;

    FileSystemModel*          m_model;
    SearchEngine              m_searchEngine;
    PathResolver*             m_pathResolver;
    TimestampedLogger*        m_logger;
    SortManager::SortCrit     m_currentSort;

    QTreeView*   m_treeView;
    QTextEdit*   m_previewPane;
    QLineEdit*   m_searchBar;
    QLineEdit*   m_addressBar;
    QListWidget* m_searchResults;
    QSplitter*   m_mainSplitter;
    QLabel*      m_statusLabel;
    QTextEdit*   m_logConsole;
};