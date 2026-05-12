#pragma once
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QIcon>
#include <QFont>
#include <QColor>
#include <stack>
#include <vector>
#include <memory>
#include "../filesystem/Folder.h"
#include "../filesystem/File.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include "../interfaces/IFileManager.h"

class FileSystemModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit FileSystemModel(IFileManager& fm,
                             const std::string& username,
                             int rootId,
                             QObject* parent = nullptr);

    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex())    const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    FileSystemEntity* entityFromIndex(const QModelIndex& index) const;

    void navigateTo(int folderId);
    void goBack();
    bool canGoBack() const;
    int currentFolderId() const;

    void refresh();

private:
    IFileManager& m_fm;
    std::string m_username;
    int m_rootId;
    int m_currentFolderId;
    std::stack<int> m_history;
    std::vector<std::shared_ptr<FileSystemEntity>> m_children;

    void loadChildren(bool filterPermissions);
};