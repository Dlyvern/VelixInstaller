#include "ProjectsWidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QLineEdit>
#include <QCoreApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QProcess>

#include <fstream>
#include <filesystem>
#include <algorithm>

#include "FireButton.hpp"
#include "widgets/VelixText.hpp"

namespace
{
QString normalizedAbsolutePath(const QString& path)
{
    return QDir(path).absolutePath();
}

QString resolveProjectTemplateDir()
{
    const QString fromCwd = QDir::current().filePath("resources/project_template");
    if (QFileInfo::exists(fromCwd))
        return normalizedAbsolutePath(fromCwd);

    const QString fromAppDir = QDir(QCoreApplication::applicationDirPath()).filePath("resources/project_template");
    if (QFileInfo::exists(fromAppDir))
        return normalizedAbsolutePath(fromAppDir);

    return {};
}

bool copyDirectoryTree(const std::filesystem::path& source, const std::filesystem::path& destination)
{
    std::error_code ec;
    std::filesystem::create_directories(destination, ec);
    if (ec)
        return false;

    for (std::filesystem::recursive_directory_iterator it(source, ec), end; it != end && !ec; it.increment(ec))
    {
        const auto& entry = *it;
        const std::filesystem::path relative = std::filesystem::relative(entry.path(), source, ec);
        if (ec)
            return false;

        const std::filesystem::path target = destination / relative;

        if (entry.is_directory())
        {
            std::filesystem::create_directories(target, ec);
            if (ec)
                return false;
            continue;
        }

        if (!entry.is_regular_file())
            continue;

        std::filesystem::create_directories(target.parent_path(), ec);
        if (ec)
            return false;

        std::filesystem::copy_file(entry.path(), target, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec)
            return false;
    }

    return !ec;
}
}

ProjectsWidget::ProjectsWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);

    m_config.load();

    auto* mainLayout  = new QVBoxLayout(this);
    mainLayout->setContentsMargins(6, 4, 6, 6);
    mainLayout->setSpacing(10);

    auto* headerLayout = new QHBoxLayout();

    auto* titleLabel = new VelixText{"Your projects", this};
    titleLabel->setPointSize(12);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);

    auto* openExistingButton = new FireButton("Open Existing", FireButton::Variant::Secondary, this);
    openExistingButton->setFixedWidth(145);
    connect(openExistingButton, &QPushButton::clicked, this, &ProjectsWidget::onOpenProjectRequested);

    auto* createButton = new FireButton("Create Project", FireButton::Variant::Primary, this);
    createButton->setFixedWidth(145);
    connect(createButton, &QPushButton::clicked, this, &ProjectsWidget::onCreateProjectRequested);

    headerLayout->addWidget(openExistingButton);
    headerLayout->addWidget(createButton);

    mainLayout->addLayout(headerLayout);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_projectsContentWidget = new QWidget(scrollArea);
    m_projectsContentWidget->setAutoFillBackground(true);
    QPalette contentPalette = m_projectsContentWidget->palette();
    contentPalette.setColor(QPalette::Window, QColor(23, 23, 23));
    m_projectsContentWidget->setPalette(contentPalette);

    QPalette viewportPalette = scrollArea->viewport()->palette();
    viewportPalette.setColor(QPalette::Window, QColor(23, 23, 23));
    scrollArea->viewport()->setPalette(viewportPalette);
    scrollArea->viewport()->setAutoFillBackground(true);

    m_projectsLayout = new QVBoxLayout(m_projectsContentWidget);
    m_projectsLayout->setSpacing(8);
    m_projectsLayout->setContentsMargins(10, 10, 10, 10);
    m_projectsLayout->addStretch(1);

    scrollArea->setWidget(m_projectsContentWidget);

    mainLayout->addWidget(scrollArea);

    loadProjectsFromConfig();
}

void ProjectsWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(bounds, 12, 12);

    QLinearGradient gradient(bounds.topLeft(), bounds.bottomLeft());
    gradient.setColorAt(0.0, QColor(20, 20, 20, 240));
    gradient.setColorAt(1.0, QColor(14, 14, 14, 240));
    painter.fillPath(path, gradient);

    painter.setPen(QPen(QColor(62, 62, 62), 1));
    painter.drawPath(path);
}

QString ProjectsWidget::defaultProjectsRoot() const
{
    const auto& config = m_config.getConfig();
    if (config.contains("project_root") && config["project_root"].is_string())
        return QString::fromStdString(config["project_root"].get<std::string>());

    return QDir::home().filePath("Documents/ElixProjects");
}

bool ProjectsWidget::createProject(const QString& parentDir, const QString& projectName, project::ProjectData& outProject)
{
    const QString trimmedName = projectName.trimmed();
    if (trimmedName.isEmpty())
        return false;

    const QString projectDir = QDir(parentDir).filePath(trimmedName);
    if (QFileInfo::exists(projectDir))
    {
        qWarning() << "Project already exists:" << projectDir;
        return false;
    }

    const QString templateDir = resolveProjectTemplateDir();
    if (templateDir.isEmpty())
    {
        qWarning() << "Could not find resources/project_template";
        return false;
    }

    const std::filesystem::path projectPathFs = projectDir.toStdString();
    std::filesystem::create_directories(projectPathFs);
    if (!copyDirectoryTree(std::filesystem::path(templateDir.toStdString()), projectPathFs))
    {
        qWarning() << "Failed to copy project templates from" << templateDir;
        return false;
    }

    std::filesystem::create_directories(projectPathFs / "Resources");
    std::filesystem::create_directories(projectPathFs / "Sources");
    std::filesystem::create_directories(projectPathFs / "build");
    std::filesystem::remove(projectPathFs / "Resources" / ".gitkeep");

    const QString projectFilePath = QDir(projectDir).filePath("project.elixproject");

    const QString absoluteProjectPath = normalizedAbsolutePath(projectDir);
    const QString projectPathWithSlash = absoluteProjectPath.endsWith('/') ? absoluteProjectPath : absoluteProjectPath + "/";

    nlohmann::json projectFileJson = {
        {"scene", QDir(absoluteProjectPath).filePath("default_scene.scene").toStdString()},
        {"path", projectPathWithSlash.toStdString()},
        {"resources_path", QDir(absoluteProjectPath).filePath("Resources").toStdString()},
        {"sources_path", QDir(absoluteProjectPath).filePath("Sources").toStdString()},
        {"name", trimmedName.toStdString()}
    };

    {
        std::ofstream projectFile(projectFilePath.toStdString());
        if (!projectFile)
            return false;

        projectFile << projectFileJson.dump(4);
    }

    outProject.name = trimmedName.toStdString();
    outProject.path = projectPathWithSlash.toStdString();
    outProject.projectFilePath = QDir(absoluteProjectPath).filePath("project.elixproject").toStdString();

    return true;
}

bool ProjectsWidget::loadProjectFile(const QString& projectFilePath, project::ProjectData& outProject) const
{
    std::ifstream ifs(projectFilePath.toStdString());
    if (!ifs)
        return false;

    nlohmann::json doc;
    try
    {
        ifs >> doc;
    }
    catch (const std::exception&)
    {
        return false;
    }

    if (!doc.contains("name") || !doc["name"].is_string() ||
        !doc.contains("path") || !doc["path"].is_string())
    {
        return false;
    }

    QString projectPath = QString::fromStdString(doc["path"].get<std::string>());
    if (projectPath.isEmpty())
        return false;

    projectPath = normalizedAbsolutePath(projectPath);
    if (!projectPath.endsWith('/'))
        projectPath += "/";

    outProject.name = doc["name"].get<std::string>();
    outProject.path = projectPath.toStdString();
    outProject.projectFilePath = QFileInfo(projectFilePath).absoluteFilePath().toStdString();

    return true;
}

bool ProjectsWidget::upsertProjectInConfig(const project::ProjectData& projectData)
{
    auto& config = m_config.mutableConfig();

    if (!config.contains("projects") || !config["projects"].is_array())
        config["projects"] = nlohmann::json::array();

    nlohmann::json entry = {
        {"name", projectData.name},
        {"path", projectData.path},
        {"project_file", projectData.projectFilePath},
        {"updated_at", QDateTime::currentDateTime().toString(Qt::ISODate).toStdString()}
    };

    bool updated = false;
    for (auto& item : config["projects"])
    {
        if (item.contains("project_file") && item["project_file"].is_string() &&
            item["project_file"] == projectData.projectFilePath)
        {
            item = entry;
            updated = true;
            break;
        }
    }

    if (!updated)
        config["projects"].push_back(entry);

    QDir projectDir(normalizedAbsolutePath(QString::fromStdString(projectData.path)));
    if (projectDir.cdUp())
        config["project_root"] = projectDir.absolutePath().toStdString();

    return m_config.save();
}

void ProjectsWidget::addProjectCard(const project::ProjectData& projectData)
{
    const QString normalizedPath = normalizedAbsolutePath(QString::fromStdString(projectData.path));
    if (m_knownProjectPaths.contains(normalizedPath))
        return;

    auto* projectCard = new ProjectWidget(projectData, this);
    connect(projectCard, &ProjectWidget::openRequested, this, &ProjectsWidget::onOpenProjectPath);

    const int insertIndex = std::max(0, m_projectsLayout->count() - 1);
    m_projectsLayout->insertWidget(insertIndex, projectCard);

    m_projectWidgets.push_back(projectCard);
    m_knownProjectPaths.insert(normalizedPath);
}

void ProjectsWidget::loadProjectsFromConfig()
{
    const auto& config = m_config.getConfig();
    if (!config.contains("projects") || !config["projects"].is_array())
        return;

    for (const auto& item : config["projects"])
    {
        if (!item.is_object() || !item.contains("project_file") || !item["project_file"].is_string())
            continue;

        project::ProjectData projectData;
        const QString projectFile = QString::fromStdString(item["project_file"].get<std::string>());
        if (!loadProjectFile(projectFile, projectData))
            continue;

        addProjectCard(projectData);
    }
}

void ProjectsWidget::onCreateProjectRequested()
{
    const QString rootDir = QFileDialog::getExistingDirectory(
        this,
        "Choose parent directory for new project",
        defaultProjectsRoot(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (rootDir.isEmpty())
        return;

    bool ok = false;
    const QString projectName = QInputDialog::getText(
        this,
        "New Project",
        "Project name:",
        QLineEdit::Normal,
        "NewProject",
        &ok
    );

    if (!ok || projectName.trimmed().isEmpty())
        return;

    project::ProjectData projectData;
    if (!createProject(rootDir, projectName, projectData))
    {
        QMessageBox::warning(this, "Create Project", "Failed to create project. Check path and permissions.");
        return;
    }

    addProjectCard(projectData);
    upsertProjectInConfig(projectData);
}

void ProjectsWidget::onOpenProjectRequested()
{
    const QString projectFile = QFileDialog::getOpenFileName(
        this,
        "Open project.elixproject",
        defaultProjectsRoot(),
        "Elix Project (*.elixproject *.elixirproject);;All Files (*)"
    );

    if (projectFile.isEmpty())
        return;

    project::ProjectData projectData;
    if (!loadProjectFile(projectFile, projectData))
    {
        QMessageBox::warning(this, "Open Project", "Invalid project file format.");
        return;
    }

    addProjectCard(projectData);
    upsertProjectInConfig(projectData);
}

void ProjectsWidget::onOpenProjectPath(const QString& projectPath)
{
    m_config.load();

    const auto& config = m_config.getConfig();
    if (!config.contains("installed_versions") || !config["installed_versions"].is_array() || config["installed_versions"].empty())
    {
        QMessageBox::warning(this, "Open Project", "No installed Velix versions found.");
        return;
    }

    QString currentVersion;
    if (config.contains("current_version") && config["current_version"].is_string())
        currentVersion = QString::fromStdString(config["current_version"].get<std::string>());

    QString selectedVersion;
    QString selectedPath;

    for (const auto& item : config["installed_versions"])
    {
        if (!item.is_object() || !item.contains("version") || !item.contains("path"))
            continue;
        if (!item["version"].is_string() || !item["path"].is_string())
            continue;

        const QString version = QString::fromStdString(item["version"].get<std::string>());
        const QString path = QString::fromStdString(item["path"].get<std::string>());

        if (!currentVersion.isEmpty() && version == currentVersion)
        {
            selectedVersion = version;
            selectedPath = path;
            break;
        }

        if (selectedVersion.isEmpty())
        {
            selectedVersion = version;
            selectedPath = path;
        }
    }

    if (selectedPath.isEmpty())
    {
        QMessageBox::warning(this, "Open Project", "Could not resolve installed version path from config.");
        return;
    }

    const QString executablePath = resolveExecutableFromInstallPath(selectedPath);
    if (executablePath.isEmpty())
    {
        QMessageBox::warning(
            this,
            "Open Project",
            QString("Could not find Velix executable for version %1.\nInstall path: %2")
                .arg(selectedVersion, selectedPath)
        );
        return;
    }

    if (!QProcess::startDetached(executablePath, {projectPath}))
    {
        QMessageBox::warning(
            this,
            "Open Project",
            QString("Failed to launch Velix executable:\n%1").arg(executablePath)
        );
    }
}

QString ProjectsWidget::resolveExecutableFromInstallPath(const QString& installPath) const
{
    QFileInfo installInfo(installPath);
    if (!installInfo.exists())
        return {};

    if (installInfo.isFile())
    {
        if (installInfo.isExecutable())
            return installInfo.absoluteFilePath();
        return {};
    }

    const QDir installDir(installInfo.absoluteFilePath());
    const QStringList directCandidates = {
        installDir.filePath("Velix"),
        installDir.filePath("Velix.exe"),
        installDir.filePath("bin/Velix"),
        installDir.filePath("bin/Velix.exe")
    };

    for (const QString& candidate : directCandidates)
    {
        QFileInfo candidateInfo(candidate);
        if (!candidateInfo.exists() || !candidateInfo.isFile())
            continue;

        if (candidateInfo.isExecutable() || candidateInfo.fileName().toLower().endsWith(".exe"))
            return candidateInfo.absoluteFilePath();
    }

    std::error_code ec;
    const std::filesystem::path root = installDir.absolutePath().toStdString();
    for (std::filesystem::recursive_directory_iterator it(root, ec), end; it != end && !ec; it.increment(ec))
    {
        if (it.depth() > 6)
        {
            it.disable_recursion_pending();
            continue;
        }

        if (!it->is_regular_file())
            continue;

        const QString fileName = QString::fromStdString(it->path().filename().string()).toLower();
        if (fileName != "velix" && fileName != "velix.exe")
            continue;

        const QFileInfo foundInfo(QString::fromStdString(it->path().string()));
        if (foundInfo.isExecutable() || fileName.endsWith(".exe"))
            return foundInfo.absoluteFilePath();
    }

    return {};
}
