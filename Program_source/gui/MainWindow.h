#pragma once
#include <QMainWindow>
#include <QTreeView>
#include <QTextEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>
#include <QStatusBar>
#include "../filesystem/Folder.h"
#include "../services/SearchEngine.h"
#include "FileSystemModel.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(Folder* root, QWidget* parent = nullptr);

private slots:
    void onItemSelected(const QModelIndex& index);
    void onSearchTriggered();
    void onSearchResultClicked(QListWidgetItem* item);

private:
    void setupUI();
    void setupMenuBar();
    void showPreview(FileSystemEntity* entity);

    Folder* m_root;
    FileSystemModel* m_model;
    SearchEngine m_searchEngine;

    QTreeView*   m_treeView;
    QTextEdit*   m_previewPane;
    QLineEdit*   m_searchBar;
    QListWidget* m_searchResults;
    QSplitter*   m_mainSplitter;
    QLabel*      m_statusBar;
};