#include "MediaWorkshopDock.hpp"

#include "MediaWorkshopQueue.hpp"
#include "PlaylistBridge.hpp"

#include <OBSApp.hpp>
#include <qt-wrappers.hpp>

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTabWidget>
#include <QVBoxLayout>

#include "moc_MediaWorkshopDock.cpp"

namespace {
constexpr int JobIdRole = Qt::UserRole + 1;

QDoubleSpinBox *CreateSecondsSpin(QWidget *parent)
{
	auto *spin = new QDoubleSpinBox(parent);
	spin->setRange(0.0, 86400.0);
	spin->setDecimals(2);
	spin->setSingleStep(0.5);
	spin->setSuffix(QStringLiteral(" s"));
	return spin;
}
}

MediaWorkshopDock::MediaWorkshopDock(QWidget *parent) : OBSDock(parent), queue(new MediaWorkshopQueue(this))
{
	setObjectName(QStringLiteral("mediaWorkshopDock"));
	setWindowTitle(QTStr("MediaWorkshop.Title"));
	setMinimumSize(640, 480);

	auto *content = new QWidget(this);
	auto *layout = new QVBoxLayout(content);
	layout->setContentsMargins(8, 8, 8, 8);
	layout->setSpacing(8);

	auto *toolbar = new QHBoxLayout();
	toolbar->setSpacing(6);
	addFilesButton = new QPushButton(QIcon(QStringLiteral(":/res/images/plus.svg")), QTStr("MediaWorkshop.AddFiles"),
					 content);
	selectOutputButton = new QPushButton(QIcon(QStringLiteral(":/res/images/save.svg")),
					     QTStr("MediaWorkshop.SelectOutput"), content);
	toolbar->addWidget(addFilesButton);
	toolbar->addWidget(selectOutputButton);
	toolbar->addStretch();
	layout->addLayout(toolbar);

	auto *outputLayout = new QHBoxLayout();
	outputLayout->setSpacing(6);
	outputLayout->addWidget(new QLabel(QTStr("MediaWorkshop.OutputDirectory"), content));
	outputDirectoryEdit = new QLineEdit(content);
	outputDirectoryEdit->setReadOnly(true);
	outputDirectoryEdit->setPlaceholderText(QTStr("MediaWorkshop.NoOutputDirectory"));
	outputLayout->addWidget(outputDirectoryEdit, 1);
	layout->addLayout(outputLayout);

	auto *tabs = new QTabWidget(content);
	auto *queuePage = new QWidget(tabs);
	auto *queueLayout = new QVBoxLayout(queuePage);
	queueLayout->setContentsMargins(0, 6, 0, 0);
	inputTable = new QTableWidget(0, 4, queuePage);
	inputTable->setHorizontalHeaderLabels({QTStr("MediaWorkshop.Column.File"), QTStr("MediaWorkshop.Column.Status"),
					    QTStr("MediaWorkshop.Column.Progress"), QTStr("MediaWorkshop.Column.Output")});
	inputTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	inputTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
	inputTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	inputTable->setAlternatingRowColors(true);
	inputTable->verticalHeader()->setVisible(false);
	inputTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
	inputTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	inputTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	inputTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
	queueLayout->addWidget(inputTable, 1);

	auto *footer = new QHBoxLayout();
	footer->setSpacing(6);
	summaryLabel = new QLabel(queuePage);
	footer->addWidget(summaryLabel);
	footer->addStretch();
	removeButton = new QPushButton(QIcon(QStringLiteral(":/res/images/minus.svg")),
				       QTStr("MediaWorkshop.RemoveSelected"), queuePage);
	clearButton = new QPushButton(QIcon(QStringLiteral(":/res/images/trash.svg")), QTStr("MediaWorkshop.Clear"),
				      queuePage);
	footer->addWidget(removeButton);
	footer->addWidget(clearButton);
	queueLayout->addLayout(footer);
	tabs->addTab(queuePage, QTStr("MediaWorkshop.Tab.Queue"));

	auto *settingsScroll = new QScrollArea(tabs);
	settingsScroll->setWidgetResizable(true);
	settingsScroll->setFrameShape(QFrame::NoFrame);
	auto *settingsPage = new QWidget(settingsScroll);
	auto *settingsLayout = new QVBoxLayout(settingsPage);
	settingsLayout->setContentsMargins(0, 6, 6, 6);
	settingsLayout->setSpacing(8);

	auto *videoGroup = new QGroupBox(QTStr("MediaWorkshop.VideoSettings"), settingsPage);
	auto *videoGrid = new QGridLayout(videoGroup);
	trimStartSpin = CreateSecondsSpin(videoGroup);
	trimEndSpin = CreateSecondsSpin(videoGroup);
	widthSpin = new QSpinBox(videoGroup);
	widthSpin->setRange(16, 7680);
	widthSpin->setSingleStep(2);
	widthSpin->setValue(1280);
	heightSpin = new QSpinBox(videoGroup);
	heightSpin->setRange(16, 4320);
	heightSpin->setSingleStep(2);
	heightSpin->setValue(720);
	fpsSpin = new QSpinBox(videoGroup);
	fpsSpin->setRange(1, 120);
	fpsSpin->setValue(30);
	crfSpin = new QSpinBox(videoGroup);
	crfSpin->setRange(0, 51);
	crfSpin->setValue(23);
	videoGrid->addWidget(new QLabel(QTStr("MediaWorkshop.TrimStart"), videoGroup), 0, 0);
	videoGrid->addWidget(trimStartSpin, 0, 1);
	videoGrid->addWidget(new QLabel(QTStr("MediaWorkshop.TrimEnd"), videoGroup), 0, 2);
	videoGrid->addWidget(trimEndSpin, 0, 3);
	videoGrid->addWidget(new QLabel(QTStr("MediaWorkshop.Width"), videoGroup), 1, 0);
	videoGrid->addWidget(widthSpin, 1, 1);
	videoGrid->addWidget(new QLabel(QTStr("MediaWorkshop.Height"), videoGroup), 1, 2);
	videoGrid->addWidget(heightSpin, 1, 3);
	videoGrid->addWidget(new QLabel(QTStr("MediaWorkshop.FPS"), videoGroup), 2, 0);
	videoGrid->addWidget(fpsSpin, 2, 1);
	videoGrid->addWidget(new QLabel(QTStr("MediaWorkshop.CRF"), videoGroup), 2, 2);
	videoGrid->addWidget(crfSpin, 2, 3);
	settingsLayout->addWidget(videoGroup);

	for (int index = 0; index < 2; ++index) {
		overlays[index] = CreateOverlayControls(settingsPage, index + 1);
		settingsLayout->addWidget(overlays[index].details);
	}

	auto *audioGroup = new QGroupBox(QTStr("MediaWorkshop.AudioSettings"), settingsPage);
	auto *audioGrid = new QGridLayout(audioGroup);
	volumeSpin = new QDoubleSpinBox(audioGroup);
	volumeSpin->setRange(-60.0, 24.0);
	volumeSpin->setDecimals(1);
	volumeSpin->setSuffix(QStringLiteral(" dB"));
	fadeInSpin = CreateSecondsSpin(audioGroup);
	fadeOutSpin = CreateSecondsSpin(audioGroup);
	audioBitrateSpin = new QSpinBox(audioGroup);
	audioBitrateSpin->setRange(32, 512);
	audioBitrateSpin->setValue(192);
	audioBitrateSpin->setSuffix(QStringLiteral(" kbps"));
	normalizeCheck = new QCheckBox(QTStr("MediaWorkshop.Normalize"), audioGroup);
	audioGrid->addWidget(new QLabel(QTStr("MediaWorkshop.Volume"), audioGroup), 0, 0);
	audioGrid->addWidget(volumeSpin, 0, 1);
	audioGrid->addWidget(new QLabel(QTStr("MediaWorkshop.AudioBitrate"), audioGroup), 0, 2);
	audioGrid->addWidget(audioBitrateSpin, 0, 3);
	audioGrid->addWidget(new QLabel(QTStr("MediaWorkshop.FadeIn"), audioGroup), 1, 0);
	audioGrid->addWidget(fadeInSpin, 1, 1);
	audioGrid->addWidget(new QLabel(QTStr("MediaWorkshop.FadeOut"), audioGroup), 1, 2);
	audioGrid->addWidget(fadeOutSpin, 1, 3);
	audioGrid->addWidget(normalizeCheck, 2, 0, 1, 4);
	settingsLayout->addWidget(audioGroup);

	auto *playlistGroup = new QGroupBox(QTStr("MediaWorkshop.Playlist"), settingsPage);
	auto *playlistLayout = new QHBoxLayout(playlistGroup);
	appendPlaylistCheck = new QCheckBox(QTStr("MediaWorkshop.AppendPlaylist"), playlistGroup);
	playlistSourceCombo = new QComboBox(playlistGroup);
	refreshPlaylistButton = new QPushButton(QTStr("MediaWorkshop.Refresh"), playlistGroup);
	refreshPlaylistButton->setToolTip(QTStr("MediaWorkshop.RefreshPlaylist"));
	playlistLayout->addWidget(appendPlaylistCheck);
	playlistLayout->addWidget(playlistSourceCombo, 1);
	playlistLayout->addWidget(refreshPlaylistButton);
	settingsLayout->addWidget(playlistGroup);
	settingsLayout->addStretch();
	settingsScroll->setWidget(settingsPage);
	tabs->addTab(settingsScroll, QTStr("MediaWorkshop.Tab.Settings"));
	layout->addWidget(tabs, 1);

	auto *actions = new QHBoxLayout();
	actions->addStretch();
	cancelButton = new QPushButton(QIcon(QStringLiteral(":/res/images/stop.svg")),
				       QTStr("MediaWorkshop.CancelCurrent"), content);
	startButton = new QPushButton(QIcon(QStringLiteral(":/res/images/media-play.svg")),
				      QTStr("MediaWorkshop.Start"), content);
	actions->addWidget(cancelButton);
	actions->addWidget(startButton);
	layout->addLayout(actions);
	setWidget(content);

	connect(addFilesButton, &QPushButton::clicked, this, &MediaWorkshopDock::SelectInputFiles);
	connect(selectOutputButton, &QPushButton::clicked, this, &MediaWorkshopDock::SelectOutputDirectory);
	connect(removeButton, &QPushButton::clicked, this, &MediaWorkshopDock::RemoveSelectedFiles);
	connect(clearButton, &QPushButton::clicked, this, &MediaWorkshopDock::ClearFiles);
	connect(startButton, &QPushButton::clicked, this, &MediaWorkshopDock::StartJobs);
	connect(cancelButton, &QPushButton::clicked, queue, &MediaWorkshopQueue::CancelCurrent);
	connect(inputTable->selectionModel(), &QItemSelectionModel::selectionChanged, this,
		&MediaWorkshopDock::UpdateControls);
	connect(queue, &MediaWorkshopQueue::JobUpdated, this, &MediaWorkshopDock::UpdateJob);
	connect(queue, &MediaWorkshopQueue::JobFinished, this, &MediaWorkshopDock::JobFinished);
	connect(queue, &MediaWorkshopQueue::QueueStateChanged, this, &MediaWorkshopDock::QueueStateChanged);
	connect(refreshPlaylistButton, &QPushButton::clicked, this, &MediaWorkshopDock::RefreshPlaylistSources);
	connect(appendPlaylistCheck, &QCheckBox::toggled, playlistSourceCombo, &QComboBox::setEnabled);

	LoadSettings();
	RefreshPlaylistSources();
	UpdateControls();
}

MediaWorkshopDock::~MediaWorkshopDock()
{
	SaveSettings();
}

MediaWorkshopDock::OverlayControls MediaWorkshopDock::CreateOverlayControls(QWidget *parent, int number)
{
	OverlayControls controls;
	auto *group = new QGroupBox(QTStr("MediaWorkshop.Overlay").arg(number), parent);
	controls.details = group;
	auto *grid = new QGridLayout(group);
	controls.enabled = new QCheckBox(QTStr("MediaWorkshop.EnableOverlay"), group);
	controls.path = new QLineEdit(group);
	controls.path->setReadOnly(true);
	controls.browse = new QPushButton(QTStr("MediaWorkshop.Browse"), group);
	controls.widthRatio = new QDoubleSpinBox(group);
	controls.widthRatio->setRange(0.05, 1.0);
	controls.widthRatio->setSingleStep(0.05);
	controls.widthRatio->setValue(0.25);
	controls.widthRatio->setSuffix(QStringLiteral(" x"));
	controls.opacity = new QDoubleSpinBox(group);
	controls.opacity->setRange(0.0, 1.0);
	controls.opacity->setSingleStep(0.05);
	controls.opacity->setValue(1.0);
	controls.position = new QComboBox(group);
	controls.position->addItems({QTStr("MediaWorkshop.Position.TopLeft"), QTStr("MediaWorkshop.Position.TopRight"),
				     QTStr("MediaWorkshop.Position.BottomLeft"),
				     QTStr("MediaWorkshop.Position.BottomRight"), QTStr("MediaWorkshop.Position.Center")});
	controls.position->setCurrentIndex(1);
	controls.start = CreateSecondsSpin(group);
	controls.end = CreateSecondsSpin(group);
	grid->addWidget(controls.enabled, 0, 0);
	grid->addWidget(controls.path, 0, 1, 1, 4);
	grid->addWidget(controls.browse, 0, 5);
	grid->addWidget(new QLabel(QTStr("MediaWorkshop.OverlaySize"), group), 1, 0);
	grid->addWidget(controls.widthRatio, 1, 1);
	grid->addWidget(new QLabel(QTStr("MediaWorkshop.Opacity"), group), 1, 2);
	grid->addWidget(controls.opacity, 1, 3);
	grid->addWidget(new QLabel(QTStr("MediaWorkshop.Position"), group), 1, 4);
	grid->addWidget(controls.position, 1, 5);
	grid->addWidget(new QLabel(QTStr("MediaWorkshop.StartTime"), group), 2, 0);
	grid->addWidget(controls.start, 2, 1);
	grid->addWidget(new QLabel(QTStr("MediaWorkshop.EndTime"), group), 2, 2);
	grid->addWidget(controls.end, 2, 3);
	connect(controls.enabled, &QCheckBox::toggled, group, [controls](bool enabled) {
		controls.path->setEnabled(enabled);
		controls.browse->setEnabled(enabled);
		controls.widthRatio->setEnabled(enabled);
		controls.opacity->setEnabled(enabled);
		controls.position->setEnabled(enabled);
		controls.start->setEnabled(enabled);
		controls.end->setEnabled(enabled);
	});
	connect(controls.browse, &QPushButton::clicked, this,
		[this, pathEdit = controls.path]() { SelectOverlayFile(pathEdit); });
	controls.enabled->setChecked(false);
	controls.enabled->toggled(false);
	return controls;
}

void MediaWorkshopDock::SelectInputFiles()
{
	const QString startDirectory = outputDirectoryEdit->text().isEmpty()
				       ? QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
				       : outputDirectoryEdit->text();
	const QStringList files = QFileDialog::getOpenFileNames(this, QTStr("MediaWorkshop.SelectFiles.Title"),
							startDirectory, QTStr("MediaWorkshop.VideoFilter"));
	for (const QString &path : files)
		AddInputFile(path);
	UpdateControls();
}

void MediaWorkshopDock::SelectOutputDirectory()
{
	const QString startDirectory = outputDirectoryEdit->text().isEmpty()
				       ? QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)
				       : outputDirectoryEdit->text();
	const QString path = QFileDialog::getExistingDirectory(this, QTStr("MediaWorkshop.SelectOutput.Title"),
							       startDirectory, QFileDialog::ShowDirsOnly);
	if (!path.isEmpty())
		outputDirectoryEdit->setText(QDir::toNativeSeparators(path));
	UpdateControls();
}

void MediaWorkshopDock::SelectOverlayFile(QLineEdit *pathEdit)
{
	const QString path = QFileDialog::getOpenFileName(this, QTStr("MediaWorkshop.SelectOverlay.Title"),
		QStandardPaths::writableLocation(QStandardPaths::MoviesLocation), QTStr("MediaWorkshop.OverlayFilter"));
	if (!path.isEmpty())
		pathEdit->setText(QDir::toNativeSeparators(path));
}

void MediaWorkshopDock::RemoveSelectedFiles()
{
	const QModelIndexList selectedRows = inputTable->selectionModel()->selectedRows();
	for (auto iterator = selectedRows.crbegin(); iterator != selectedRows.crend(); ++iterator)
		inputTable->removeRow(iterator->row());
	UpdateControls();
}

void MediaWorkshopDock::ClearFiles()
{
	inputTable->setRowCount(0);
	UpdateControls();
}

void MediaWorkshopDock::StartJobs()
{
	const QFileInfo outputInfo(outputDirectoryEdit->text());
	if (!outputInfo.exists() || !outputInfo.isDir() || !outputInfo.isWritable()) {
		QMessageBox::warning(this, QTStr("MediaWorkshop.Title"), QTStr("MediaWorkshop.Error.OutputDirectory"));
		return;
	}
	if (trimEndSpin->value() > 0.0 && trimEndSpin->value() <= trimStartSpin->value()) {
		QMessageBox::warning(this, QTStr("MediaWorkshop.Title"), QTStr("MediaWorkshop.Error.TrimRange"));
		return;
	}
	for (const OverlayControls &controls : overlays) {
		if (controls.enabled->isChecked() && controls.path->text().isEmpty()) {
			QMessageBox::warning(this, QTStr("MediaWorkshop.Title"), QTStr("MediaWorkshop.Error.OverlayMissing"));
			return;
		}
	}

	QList<MediaWorkshopJob> jobs;
	QSet<QString> reservedPaths;
	for (int row = 0; row < inputTable->rowCount(); ++row) {
		auto *fileItem = inputTable->item(row, 0);
		const QString inputPath = fileItem->data(Qt::UserRole).toString();
		const QString outputPath = CreateOutputPath(inputPath, outputInfo.absoluteFilePath(), reservedPaths);
		MediaWorkshopJob job = BuildJob(inputPath, outputPath);
		fileItem->setData(JobIdRole, job.id.toString(QUuid::WithoutBraces));
		inputTable->item(row, 1)->setText(StateText(job.state));
		inputTable->item(row, 2)->setText(QStringLiteral("0%"));
		inputTable->item(row, 3)->setText(QDir::toNativeSeparators(outputPath));
		inputTable->item(row, 3)->setToolTip(QDir::toNativeSeparators(outputPath));
		jobs.push_back(job);
	}
	if (!jobs.isEmpty())
		queue->Enqueue(jobs);
}

void MediaWorkshopDock::AddInputFile(const QString &path)
{
	const QString normalizedPath = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
	for (int row = 0; row < inputTable->rowCount(); ++row) {
		const auto *item = inputTable->item(row, 0);
		if (item && QDir::cleanPath(item->data(Qt::UserRole).toString()).compare(normalizedPath,
										Qt::CaseInsensitive) == 0)
			return;
	}
	const QFileInfo fileInfo(normalizedPath);
	if (!fileInfo.exists() || !fileInfo.isFile())
		return;
	const int row = inputTable->rowCount();
	inputTable->insertRow(row);
	auto *fileItem = new QTableWidgetItem(fileInfo.fileName());
	fileItem->setData(Qt::UserRole, normalizedPath);
	fileItem->setToolTip(QDir::toNativeSeparators(normalizedPath));
	inputTable->setItem(row, 0, fileItem);
	inputTable->setItem(row, 1, new QTableWidgetItem(QTStr("MediaWorkshop.Status.Pending")));
	inputTable->setItem(row, 2, new QTableWidgetItem(QStringLiteral("0%")));
	inputTable->setItem(row, 3, new QTableWidgetItem());
}

MediaWorkshopJob MediaWorkshopDock::BuildJob(const QString &inputPath, const QString &outputPath) const
{
	MediaWorkshopJob job;
	job.inputPath = inputPath;
	job.outputPath = outputPath;
	const QFileInfo outputInfo(outputPath);
	job.temporaryPath = outputInfo.dir().filePath(
		QStringLiteral(".%1.%2.part.mp4").arg(outputInfo.completeBaseName(), job.id.toString(QUuid::WithoutBraces)));
	job.trimStartSeconds = trimStartSpin->value();
	job.trimEndSeconds = trimEndSpin->value();
	job.output.width = widthSpin->value() & ~1;
	job.output.height = heightSpin->value() & ~1;
	job.output.fps = fpsSpin->value();
	job.output.crf = crfSpin->value();
	job.output.audioBitrateKbps = audioBitrateSpin->value();
	job.audio.volumeDb = volumeSpin->value();
	job.audio.fadeInSeconds = fadeInSpin->value();
	job.audio.fadeOutSeconds = fadeOutSpin->value();
	job.audio.loudnessNormalize = normalizeCheck->isChecked();
	for (const OverlayControls &controls : overlays)
		job.overlays.push_back(BuildOverlay(controls));
	return job;
}

MediaWorkshopOverlay MediaWorkshopDock::BuildOverlay(const OverlayControls &controls) const
{
	MediaWorkshopOverlay overlay;
	overlay.enabled = controls.enabled->isChecked();
	overlay.inputPath = QDir::cleanPath(controls.path->text());
	overlay.widthRatio = controls.widthRatio->value();
	overlay.opacity = controls.opacity->value();
	overlay.position = static_cast<MediaWorkshopOverlayPosition>(controls.position->currentIndex());
	overlay.startSeconds = controls.start->value();
	overlay.endSeconds = controls.end->value();
	return overlay;
}

QString MediaWorkshopDock::CreateOutputPath(const QString &inputPath, const QString &outputDirectory,
					    QSet<QString> &reservedPaths) const
{
	const QString base = QFileInfo(inputPath).completeBaseName() + QStringLiteral("_processed");
	int suffix = 0;
	QString candidate;
	do {
		const QString name = suffix == 0 ? base + QStringLiteral(".mp4")
						 : QStringLiteral("%1_%2.mp4").arg(base).arg(suffix);
		candidate = QDir(outputDirectory).filePath(name);
		++suffix;
	} while (QFileInfo::exists(candidate) || reservedPaths.contains(candidate.toCaseFolded()));
	reservedPaths.insert(candidate.toCaseFolded());
	return QDir::cleanPath(candidate);
}

void MediaWorkshopDock::UpdateJob(const MediaWorkshopJob &job)
{
	const QString id = job.id.toString(QUuid::WithoutBraces);
	for (int row = 0; row < inputTable->rowCount(); ++row) {
		auto *fileItem = inputTable->item(row, 0);
		if (fileItem->data(JobIdRole).toString() != id)
			continue;
		inputTable->item(row, 1)->setText(StateText(job.state));
		inputTable->item(row, 1)->setToolTip(job.errorSummary);
		inputTable->item(row, 2)->setText(QStringLiteral("%1%").arg(qRound(job.progress * 100.0)));
		break;
	}
}

void MediaWorkshopDock::QueueStateChanged(bool busy)
{
	queueBusy = busy;
	UpdateControls();
}

void MediaWorkshopDock::JobFinished(const MediaWorkshopJob &job)
{
	if (job.state != MediaWorkshopJobState::Succeeded || !appendPlaylistCheck->isChecked() ||
	    playlistSourceCombo->currentText().isEmpty())
		return;
	const PlaylistAppendResult result = PlaylistBridge::Append(playlistSourceCombo->currentText(), job.outputPath);
	if (result == PlaylistAppendResult::SourceNotFound || result == PlaylistAppendResult::WrongSourceType ||
	    result == PlaylistAppendResult::UpdateFailed) {
		QMessageBox::warning(this, QTStr("MediaWorkshop.Title"), QTStr("MediaWorkshop.Error.PlaylistUpdate"));
		RefreshPlaylistSources();
	}
}

void MediaWorkshopDock::RefreshPlaylistSources()
{
	QString selected = playlistSourceCombo->currentText();
	if (selected.isEmpty())
		selected = playlistSourceCombo->property("savedSource").toString();
	playlistSourceCombo->clear();
	playlistSourceCombo->addItems(PlaylistBridge::SourceNames());
	const int selectedIndex = playlistSourceCombo->findText(selected);
	if (selectedIndex >= 0)
		playlistSourceCombo->setCurrentIndex(selectedIndex);
	playlistSourceCombo->setProperty("savedSource", QVariant());
	playlistSourceCombo->setEnabled(appendPlaylistCheck->isChecked());
}

void MediaWorkshopDock::UpdateControls()
{
	const int count = inputTable->rowCount();
	summaryLabel->setText(QTStr("MediaWorkshop.Summary").arg(count));
	addFilesButton->setEnabled(!queueBusy);
	selectOutputButton->setEnabled(!queueBusy);
	removeButton->setEnabled(!queueBusy && inputTable->selectionModel()->hasSelection());
	clearButton->setEnabled(!queueBusy && count > 0);
	startButton->setEnabled(!queueBusy && count > 0 && !outputDirectoryEdit->text().isEmpty());
	cancelButton->setEnabled(queueBusy);
}

QString MediaWorkshopDock::StateText(MediaWorkshopJobState state) const
{
	switch (state) {
	case MediaWorkshopJobState::Pending: return QTStr("MediaWorkshop.Status.Pending");
	case MediaWorkshopJobState::Probing: return QTStr("MediaWorkshop.Status.Probing");
	case MediaWorkshopJobState::Running: return QTStr("MediaWorkshop.Status.Running");
	case MediaWorkshopJobState::Validating: return QTStr("MediaWorkshop.Status.Validating");
	case MediaWorkshopJobState::Succeeded: return QTStr("MediaWorkshop.Status.Succeeded");
	case MediaWorkshopJobState::Failed: return QTStr("MediaWorkshop.Status.Failed");
	case MediaWorkshopJobState::Cancelling: return QTStr("MediaWorkshop.Status.Cancelling");
	case MediaWorkshopJobState::Cancelled: return QTStr("MediaWorkshop.Status.Cancelled");
	}
	return QTStr("MediaWorkshop.Status.Failed");
}

bool MediaWorkshopDock::ConfirmShutdown()
{
	if (!queue->IsBusy())
		return true;
	QMessageBox box(QMessageBox::Question, QTStr("MediaWorkshop.Exit.Title"), QTStr("MediaWorkshop.Exit.Text"),
			QMessageBox::NoButton, this);
	auto *waitButton = box.addButton(QTStr("MediaWorkshop.Exit.Wait"), QMessageBox::RejectRole);
	auto *cancelButton = box.addButton(QTStr("MediaWorkshop.Exit.CancelTasks"), QMessageBox::DestructiveRole);
	box.exec();
	if (box.clickedButton() == waitButton)
		return false;
	if (box.clickedButton() == cancelButton) {
		queue->CancelAll();
		return true;
	}
	return false;
}

void MediaWorkshopDock::LoadSettings()
{
	config_t *config = App()->GetUserConfig();
	const bool hasSettings = config_get_int(config, "MediaWorkshop", "SchemaVersion") >= 1;
	const char *output = config_get_string(config, "MediaWorkshop", "OutputDirectory");
	if (output && *output)
		outputDirectoryEdit->setText(QDir::toNativeSeparators(QString::fromUtf8(output)));
	const int width = static_cast<int>(config_get_int(config, "MediaWorkshop", "Width"));
	const int height = static_cast<int>(config_get_int(config, "MediaWorkshop", "Height"));
	widthSpin->setValue(width >= 16 ? width : 1280);
	heightSpin->setValue(height >= 16 ? height : 720);
	const int crf = static_cast<int>(config_get_int(config, "MediaWorkshop", "CRF"));
	crfSpin->setValue(crf >= 0 && crf <= 51 ? crf : 23);
	const int fps = static_cast<int>(config_get_int(config, "MediaWorkshop", "FPS"));
	fpsSpin->setValue(fps >= 1 && fps <= 120 ? fps : 30);
	const int bitrate = static_cast<int>(config_get_int(config, "MediaWorkshop", "AudioBitrate"));
	audioBitrateSpin->setValue(bitrate >= 32 ? bitrate : 192);
	trimStartSpin->setValue(config_get_double(config, "MediaWorkshop", "TrimStart"));
	trimEndSpin->setValue(config_get_double(config, "MediaWorkshop", "TrimEnd"));
	volumeSpin->setValue(config_get_double(config, "MediaWorkshop", "VolumeDb"));
	fadeInSpin->setValue(config_get_double(config, "MediaWorkshop", "FadeIn"));
	fadeOutSpin->setValue(config_get_double(config, "MediaWorkshop", "FadeOut"));
	normalizeCheck->setChecked(config_get_bool(config, "MediaWorkshop", "Normalize"));
	for (int index = 0; index < 2; ++index) {
		const QByteArray prefix = QStringLiteral("Overlay%1").arg(index + 1).toUtf8();
		const auto key = [&prefix](const char *suffix) { return prefix + suffix; };
		overlays[index].enabled->setChecked(config_get_bool(config, "MediaWorkshop", key("Enabled").constData()));
		const char *path = config_get_string(config, "MediaWorkshop", key("Path").constData());
		overlays[index].path->setText(path ? QDir::toNativeSeparators(QString::fromUtf8(path)) : QString());
		const double ratio = config_get_double(config, "MediaWorkshop", key("Ratio").constData());
		const double opacity = config_get_double(config, "MediaWorkshop", key("Opacity").constData());
		overlays[index].widthRatio->setValue(hasSettings && ratio > 0.0 ? ratio : 0.25);
		overlays[index].opacity->setValue(hasSettings && opacity >= 0.0 && opacity <= 1.0 ? opacity : 1.0);
		overlays[index].position->setCurrentIndex(hasSettings ? qBound(0, static_cast<int>(config_get_int(
			config, "MediaWorkshop", key("Position").constData())), 4) : 1);
		overlays[index].start->setValue(config_get_double(config, "MediaWorkshop", key("Start").constData()));
		overlays[index].end->setValue(config_get_double(config, "MediaWorkshop", key("End").constData()));
	}
	appendPlaylistCheck->setChecked(config_get_bool(config, "MediaWorkshop", "AppendPlaylist"));
	const char *savedSource = config_get_string(config, "MediaWorkshop", "PlaylistSource");
	playlistSourceCombo->setProperty("savedSource", savedSource ? QString::fromUtf8(savedSource) : QString());
}

void MediaWorkshopDock::SaveSettings() const
{
	config_t *config = App()->GetUserConfig();
	config_set_int(config, "MediaWorkshop", "SchemaVersion", 1);
	config_set_string(config, "MediaWorkshop", "OutputDirectory", outputDirectoryEdit->text().toUtf8().constData());
	config_set_int(config, "MediaWorkshop", "Width", widthSpin->value());
	config_set_int(config, "MediaWorkshop", "Height", heightSpin->value());
	config_set_int(config, "MediaWorkshop", "FPS", fpsSpin->value());
	config_set_int(config, "MediaWorkshop", "CRF", crfSpin->value());
	config_set_int(config, "MediaWorkshop", "AudioBitrate", audioBitrateSpin->value());
	config_set_double(config, "MediaWorkshop", "TrimStart", trimStartSpin->value());
	config_set_double(config, "MediaWorkshop", "TrimEnd", trimEndSpin->value());
	config_set_double(config, "MediaWorkshop", "VolumeDb", volumeSpin->value());
	config_set_double(config, "MediaWorkshop", "FadeIn", fadeInSpin->value());
	config_set_double(config, "MediaWorkshop", "FadeOut", fadeOutSpin->value());
	config_set_bool(config, "MediaWorkshop", "Normalize", normalizeCheck->isChecked());
	for (int index = 0; index < 2; ++index) {
		const QByteArray prefix = QStringLiteral("Overlay%1").arg(index + 1).toUtf8();
		const auto key = [&prefix](const char *suffix) { return prefix + suffix; };
		config_set_bool(config, "MediaWorkshop", key("Enabled").constData(), overlays[index].enabled->isChecked());
		config_set_string(config, "MediaWorkshop", key("Path").constData(),
				  overlays[index].path->text().toUtf8().constData());
		config_set_double(config, "MediaWorkshop", key("Ratio").constData(), overlays[index].widthRatio->value());
		config_set_double(config, "MediaWorkshop", key("Opacity").constData(), overlays[index].opacity->value());
		config_set_int(config, "MediaWorkshop", key("Position").constData(), overlays[index].position->currentIndex());
		config_set_double(config, "MediaWorkshop", key("Start").constData(), overlays[index].start->value());
		config_set_double(config, "MediaWorkshop", key("End").constData(), overlays[index].end->value());
	}
	config_set_bool(config, "MediaWorkshop", "AppendPlaylist", appendPlaylistCheck->isChecked());
	config_set_string(config, "MediaWorkshop", "PlaylistSource", playlistSourceCombo->currentText().toUtf8().constData());
	config_save_safe(config, "tmp", nullptr);
}
