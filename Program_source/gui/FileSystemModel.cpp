#include "FileSystemModel.h"
#include "../filesystem/TextFile.h"
#include "../filesystem/BinaryFile.h"
#include <QIcon>
#include <QFont>
#include <algorithm>

FileSystemModel::FileSystemModel(IFileManager& fm,
                                 const std::string& username,
                                 int rootId,
                                 QObject* parent)
    : QAbstractItemModel(parent), m_fm(fm),
      m_username(username), m_rootId(rootId),
      m_currentFolderId(rootId)
{
    loadChildren(false); // primul nivel — fara filtrare
}

void FileSystemModel::loadChildren(bool filterPermissions) {
    m_children = m_fm.getChildren(m_currentFolderId);

    if (filterPermissions) {
        m_children.erase(
            std::remove_if(m_children.begin(), m_children.end(),
                [&](const std::shared_ptr<FileSystemEntity>& e) {
                    return !m_fm.checkPermission(e->getId(), m_username, "read");
                }),
            m_children.end()
        );
    }
}

void FileSystemModel::navigateTo(int folderId) {
    beginResetModel();
    m_history.push(m_currentFolderId);
    m_currentFolderId = folderId;
    loadChildren(true);
    endResetModel();
}

void FileSystemModel::goBack() {
    if (m_history.empty()) return;
    m_currentFolderId = m_history.top();
    m_history.pop();
    beginResetModel();
    // daca ne intoarcem la root, fara filtrare
    loadChildren(m_currentFolderId != m_rootId);
    endResetModel();
}

bool FileSystemModel::canGoBack() const {
    return !m_history.empty();
}

int FileSystemModel::currentFolderId() const {
    return m_currentFolderId;
}

void FileSystemModel::refresh() {
    beginResetModel();
    loadChildren(m_currentFolderId != m_rootId);
    endResetModel();
}

QModelIndex FileSystemModel::index(int row, int column,
                                   const QModelIndex& parent) const {
    if (parent.isValid()) return QModelIndex();
    if (row < 0 || row >= (int)m_children.size()) return QModelIndex();
    return createIndex(row, column, m_children[row].get());
}

QModelIndex FileSystemModel::parent(const QModelIndex& child) const {
    Q_UNUSED(child);
    return QModelIndex();
}

int FileSystemModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return (int)m_children.size();
}

int FileSystemModel::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);
    return 4;
}

QVariant FileSystemModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return QVariant();
    if (index.row() < 0 || index.row() >= (int)m_children.size()) return QVariant();

    auto* entity = static_cast<FileSystemEntity*>(index.internalPointer());
    if (!entity) return QVariant();

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return QString::fromStdString(entity->getName());
        case 1: {
            if (entity->isFolder()) return "Folder";
            auto* f = dynamic_cast<File*>(entity);
            return f ? QString::fromStdString(f->getExtension()) : "";
        }
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
        if (entity->isFolder())
            return QIcon(":/icons/folder.png");

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
    if (!index.isValid()) return nullptr;
    return static_cast<FileSystemEntity*>(index.internalPointer());
}