#pragma once

#include "MediaWorkshopJob.hpp"

#include <docks/OBSDock.hpp>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QWidget;
class MediaWorkshopQueue;

class MediaWorkshopDock : public OBSDock {
	Q_OBJECT

public:
	explicit MediaWorkshopDock(QWidget *parent = nullptr);
	~MediaWorkshopDock() override;

	bool ConfirmShutdown();

private:
	struct OverlayControls {
		QCheckBox *enabled = nullptr;
		QLineEdit *path = nullptr;
		QPushButton *browse = nullptr;
		QDoubleSpinBox *widthRatio = nullptr;
		QDoubleSpinBox *opacity = nullptr;
		QComboBox *position = nullptr;
		QDoubleSpinBox *start = nullptr;
		QDoubleSpinBox *end = nullptr;
		QWidget *details = nullptr;
	};

	void SelectInputFiles();
	void SelectOutputDirectory();
	void SelectOverlayFile(QLineEdit *pathEdit);
	void RemoveSelectedFiles();
	void ClearFiles();
	void StartJobs();
	void AddInputFile(const QString &path);
	void UpdateJob(const MediaWorkshopJob &job);
	void JobFinished(const MediaWorkshopJob &job);
	void QueueStateChanged(bool busy);
	void RefreshPlaylistSources();
	void UpdateControls();
	void LoadSettings();
	void SaveSettings() const;
	MediaWorkshopJob BuildJob(const QString &inputPath, const QString &outputPath) const;
	MediaWorkshopOverlay BuildOverlay(const OverlayControls &controls) const;
	QString CreateOutputPath(const QString &inputPath, const QString &outputDirectory,
				 QSet<QString> &reservedPaths) const;
	QString StateText(MediaWorkshopJobState state) const;
	OverlayControls CreateOverlayControls(QWidget *parent, int number);

	MediaWorkshopQueue *queue = nullptr;
	QLineEdit *outputDirectoryEdit = nullptr;
	QTableWidget *inputTable = nullptr;
	QLabel *summaryLabel = nullptr;
	QPushButton *addFilesButton = nullptr;
	QPushButton *selectOutputButton = nullptr;
	QPushButton *removeButton = nullptr;
	QPushButton *clearButton = nullptr;
	QPushButton *startButton = nullptr;
	QPushButton *cancelButton = nullptr;
	QDoubleSpinBox *trimStartSpin = nullptr;
	QDoubleSpinBox *trimEndSpin = nullptr;
	QSpinBox *widthSpin = nullptr;
	QSpinBox *heightSpin = nullptr;
	QSpinBox *fpsSpin = nullptr;
	QSpinBox *crfSpin = nullptr;
	QSpinBox *audioBitrateSpin = nullptr;
	QDoubleSpinBox *volumeSpin = nullptr;
	QDoubleSpinBox *fadeInSpin = nullptr;
	QDoubleSpinBox *fadeOutSpin = nullptr;
	QCheckBox *normalizeCheck = nullptr;
	QCheckBox *appendPlaylistCheck = nullptr;
	QComboBox *playlistSourceCombo = nullptr;
	QPushButton *refreshPlaylistButton = nullptr;
	OverlayControls overlays[2];
	bool queueBusy = false;
};
