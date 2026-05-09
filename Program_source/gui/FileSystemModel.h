#pragma once
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QIcon>
#include <QFont>
#include <QColor>
#include "../filesystem/Folder.h"
#include "../filesystem/SharedFolder.h"
#include "../filesystem/File.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"

class FileSystemModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit FileSystemModel(Folder* root, QObject* parent = nullptr);

    // ── Metode obligatorii QAbstractItemModel ────────
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex& child) const override;

    int rowCount(const QModelIndex& parent = QModelIndex())   const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;

    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // ── Helper ───────────────────────────────────────
    FileSystemEntity* entityFromIndex(const QModelIndex& index) const;

    // ── Refresh model dupa modificari ────────────────
    void refresh();

private:
    Folder* m_root;

    Folder* findParent(Folder* current, FileSystemEntity* target) const;
};