#include "FileSystemModel.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include <QIcon>
#include <QFont>
#include <QColor>

FileSystemModel::FileSystemModel(Folder* root, QObject* parent)
    : QAbstractItemModel(parent), m_root(root)
{}

QModelIndex FileSystemModel::index(int row, int column,
                                   const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();

    Folder* parentFolder = nullptr;

    if (!parent.isValid()) {
        parentFolder = m_root;
    } else {
        auto* entity = static_cast<FileSystemEntity*>(parent.internalPointer());
        parentFolder = dynamic_cast<Folder*>(entity);
    }

    if (!parentFolder) return QModelIndex();

    auto children = parentFolder->getChildren();
    if (row >= (int)children.size()) return QModelIndex();

    return createIndex(row, column, children[row].get());
}

QModelIndex FileSystemModel::parent(const QModelIndex& child) const {
    if (!child.isValid()) return QModelIndex();

    auto* entity = static_cast<FileSystemEntity*>(child.internalPointer());
    if (!entity || entity == m_root) return QModelIndex();

    // gasim parintele entitatii
    Folder* parentFolder = findParent(m_root, entity);
    if (!parentFolder || parentFolder == m_root) return QModelIndex();

    // gasim parintele parintelui
    Folder* grandParent = findParent(m_root, parentFolder);
    if (!grandParent) return QModelIndex();

    // gasim indexul parintelui in bunic
    auto children = grandParent->getChildren();
    for (int i = 0; i < (int)children.size(); i++) {
        if (children[i].get() == parentFolder) {
            return createIndex(i, 0, parentFolder);
        }
    }
    return QModelIndex();
}

int FileSystemModel::rowCount(const QModelIndex& parent) const {
    Folder* parentFolder = nullptr;

    if (!parent.isValid()) {
        parentFolder = m_root;
    } else {
        auto* entity = static_cast<FileSystemEntity*>(parent.internalPointer());
        parentFolder = dynamic_cast<Folder*>(entity);
    }

    if (!parentFolder) return 0;
    return parentFolder->getChildCount();
}

int FileSystemModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return 4; // Nume | Tip | Dimensiune | Data crearii
}

QVariant FileSystemModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();

    auto* entity = static_cast<FileSystemEntity*>(index.internalPointer());
    if (!entity) return QVariant();

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return QString::fromStdString(entity->getName());
        case 1: return entity->isFolder() ? "Folder" :
                       QString::fromStdString(
                           dynamic_cast<File*>(entity) ?
                               dynamic_cast<File*>(entity)->getExtension() : "");
        case 2: return QString::number(entity->getSize()) + " B";
        case 3: {
            std::time_t t = entity->getCreatedAt();
            char buf[32];
            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", std::localtime(&t));
            return QString(buf);
        }
        default: return QVariant();
        }
    }

    if (role == Qt::DecorationRole && index.column() == 0) {
        if (entity->isFolder()) {
            auto* sf = dynamic_cast<SharedFolder*>(entity);
            return QIcon(sf ? ":/icons/folder_shared.png" : ":/icons/folder.png");
        }

        auto* file = dynamic_cast<File*>(entity);
        if (file) {
            std::string ext = file->getExtension();
            if (ext == ".txt")  return QIcon(":/icons/file_txt.png");
            if (ext == ".docx") return QIcon(":/icons/file_word.png");
            if (ext == ".pptx") return QIcon(":/icons/file_ppt.png");
            if (ext == ".jpg" || ext == ".png") return QIcon(":/icons/file_img.png");
            if (ext == ".pdf")  return QIcon(":/icons/file_pdf.png");
        }
        return QIcon(":/icons/file.png");
    }

    if (role == Qt::FontRole) {
        QFont font;
        if (entity->isFolder()) font.setBold(true);
        return font;
    }

    if (role == Qt::ForegroundRole) {
        auto* sf = dynamic_cast<SharedFolder*>(entity);
        if (sf) return QColor(0, 120, 215); // albastru pentru shared
        return QVariant();
    }

    return QVariant();
}

QVariant FileSystemModel::headerData(int section,
                                     Qt::Orientation orientation,
                                     int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return "Nume";
        case 1: return "Tip";
        case 2: return "Dimensiune";
        case 3: return "Data crearii";
        default: return QVariant();
        }
    }
    return QVariant();
}

FileSystemEntity* FileSystemModel::entityFromIndex(const QModelIndex& index) const {
    if (!index.isValid()) return m_root;
    return static_cast<FileSystemEntity*>(index.internalPointer());
}

Folder* FileSystemModel::findParent(Folder* current, FileSystemEntity* target) const {
    auto children = current->getChildren();
    for (const auto& child : children) {
        if (child.get() == target) return current;

        auto* subFolder = dynamic_cast<Folder*>(child.get());
        if (subFolder) {
            Folder* result = findParent(subFolder, target);
            if (result) return result;
        }
    }
    return nullptr;
}