#pragma once
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include "../filesystem/Folder.h"
#include "../filesystem/File.h"

class FileSystemModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit FileSystemModel(Folder* root, QObject* parent = nullptr);

    // Metodele obligatorii pentru QAbstractItemModel
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // Returneaza entitatea asociata unui index
    FileSystemEntity* entityFromIndex(const QModelIndex& index) const;

private:
    Folder* m_root;
    Folder* findParent(Folder* current, FileSystemEntity* target) const;
};