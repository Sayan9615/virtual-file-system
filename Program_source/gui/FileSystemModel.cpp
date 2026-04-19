#include "FileSystemModel.h"
#include <QIcon>

FileSystemModel::FileSystemModel(Folder* root, QObject* parent)
    : QAbstractItemModel(parent), m_root(root) {}

QModelIndex FileSystemModel::index(int row, int column,
                                    const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    Folder* parentFolder = parent.isValid()
        ? static_cast<Folder*>(parent.internalPointer())
        : m_root;

    auto children = parentFolder->getChildren();
    if (row < (int)children.size())
        return createIndex(row, column, children[row].get());

    return QModelIndex();
}

QModelIndex FileSystemModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) return QModelIndex();

    auto* entity = static_cast<FileSystemEntity*>(child.internalPointer());

    // gasim folderul parinte al acestui nod
    Folder* parentFolder = nullptr;

    if (entity->isFolder()) {
        parentFolder = static_cast<Folder*>(entity)->getParent();
    } else {
        // pentru fisiere, cautam in tot arborele cine il contine
        parentFolder = findParent(m_root, entity);
    }

    if (!parentFolder || parentFolder == m_root)
        return QModelIndex();

    // gasim row-ul parintelui in bunicul sau
    Folder* grandParent = parentFolder->getParent();
    if (!grandParent) grandParent = m_root;

    auto siblings = grandParent->getChildren();
    for (int i = 0; i < (int)siblings.size(); i++) {
        if (siblings[i].get() == parentFolder)
            return createIndex(i, 0, parentFolder);
    }
    return QModelIndex();
}

Folder* FileSystemModel::findParent(Folder* current, FileSystemEntity* target) const {
    for (const auto& child : current->getChildren()) {
        if (child.get() == target)
            return current;
        if (child->isFolder()) {
            Folder* result = findParent(static_cast<Folder*>(child.get()), target);
            if (result) return result;
        }
    }
    return nullptr;
}

int FileSystemModel::rowCount(const QModelIndex& parent) const {
    Folder* folder = parent.isValid()
        ? dynamic_cast<Folder*>(static_cast<FileSystemEntity*>(parent.internalPointer()))
        : m_root;

    return folder ? folder->getChildCount() : 0;
}

int FileSystemModel::columnCount(const QModelIndex&) const {
    return 3;
}

QVariant FileSystemModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();

    auto* entity = static_cast<FileSystemEntity*>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return QString::fromStdString(entity->getName());
            case 1: return entity->isFolder() ? "Folder" : "File";
            case 2: return QString::number(entity->getSize()) + " B";
        }
    }

    if (role == Qt::DecorationRole && index.column() == 0) {
        return entity->isFolder()
            ? QIcon::fromTheme("folder")
            : QIcon::fromTheme("text-x-generic");
    }

    return QVariant();
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return "Nume";
            case 1: return "Tip";
            case 2: return "Marime";
        }
    }
    return QVariant();
}

FileSystemEntity* FileSystemModel::entityFromIndex(const QModelIndex& index) const {
    return index.isValid()
        ? static_cast<FileSystemEntity*>(index.internalPointer())
        : nullptr;
}