#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "recentfiles.h"
#include "convert.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <QCloseEvent>
#include <QSettings>
#include <QFileDialog>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow), m_recentFiles(new RecentFiles(this))
{
	ui->setupUi(this);
	ui->actionRecent_Files->setMenu(m_recentFiles->menu());

	_CreateProcessActions();

	connect(m_recentFiles, &RecentFiles::activated, this, &MainWindow::_Open);
	connect(ui->action_Open, &QAction::triggered, [this]() {
		QString filePath = QFileDialog::getOpenFileName(this, tr("Open"), m_recentFiles->mostRecentFile(), "Images(*.png *.bmp *.jpg *.gif)");
		if (filePath.isEmpty())
			return;
		_Open(filePath);
	});
	connect(ui->action_Save, &QAction::triggered, [this]() {
		if (ui->originalView->pixmap().isNull())
			return;
		ui->originalView->pixmap().save(m_recentFiles->mostRecentFile());
	});
	connect(ui->actionSave_As, &QAction::triggered, [this]() {
		if (ui->originalView->pixmap().isNull())
			return;

		QString filePath = QFileDialog::getSaveFileName(this, tr("Save as"), m_recentFiles->mostRecentFile(), "Images(*.png *.bmp *.jpg *.gif)");
		if (filePath.isEmpty())
			return;
		ui->originalView->pixmap().save(filePath);
	});

	{
		using namespace QtOcv;

		auto colorUpdateOnMouseMoved = [this](const QColor &c) {
			if (c.isValid())
				statusBar()->showMessage(QString("R %1 G %2 B %3").arg(c.red()).arg(c.green()).arg(c.blue()));
			else
				statusBar()->clearMessage();
		};

		connect(ui->originalView, &ImageWidget::colorUnderMouseChanged, colorUpdateOnMouseMoved);
		connect(ui->processView, &ImageWidget::colorUnderMouseChanged, colorUpdateOnMouseMoved);
		connect(ui->originalView, &ImageWidget::realScaleChanged, ui->processView, &ImageWidget::setCurrentScale);
		connect(ui->processView, &ImageWidget::realScaleChanged, ui->originalView, &ImageWidget::setCurrentScale);
		connect(ui->originalView->horizontalScrollBar(), &QScrollBar::valueChanged, ui->processView, &ImageWidget::setHorizontalScrollbarValue);
		connect(ui->originalView->verticalScrollBar(), &QScrollBar::valueChanged, ui->processView, &ImageWidget::setVerticalScrollbarValue);
	}

	connect(ui->btnApply, &QPushButton::pressed, this, &MainWindow::_OnApply);
	connect(ui->btnSetBase, &QPushButton::pressed, [this]() {
		ui->originalView->setPixmap(ui->processView->pixmap());
		m_originalMat = m_processMat;
		m_processMat = cv::Mat();
		ui->processView->setPixmap(QPixmap());
	});

	ui->filterDockWidget->setEnabled(false);
	_LoadSettings();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent *evt)
{
	_SaveSettings();
	evt->accept();
}

void MainWindow::_LoadSettings()
{
	QSettings settings;
	settings.beginGroup("RecentFiles");
	m_recentFiles->setFileList(settings.value("list").toStringList());
	settings.endGroup();
}

void MainWindow::_SaveSettings()
{
	QSettings settings;
	settings.beginGroup("RecentFiles");
	settings.setValue("list", m_recentFiles->fileList());
	settings.endGroup();
}

void MainWindow::_OnApply()
{
	if (!m_convert)
		return;

	qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
	if (m_convert->applyTo(m_originalMat, m_processMat))
		ui->processView->setImage(QtOcv::mat2Image_shared(m_processMat));
	else
		statusBar()->showMessage(m_convert->errorString(), 3000);
	qApp->restoreOverrideCursor();
}

void MainWindow::_CreateProcessAction(const QString& iconName, const QString& title, std::function<AbstractConvert*()> functor)
{
	auto action = new QAction(QIcon(":/Algo/Resources/" + iconName + ".png"), title, this);
	connect(action, &QAction::triggered, [this, title, functor]() {
		if (m_originalMat.empty()) 
			return;
		
		m_convert.clear();
		m_convert = QSharedPointer<AbstractConvert>(functor());
		
		ui->processView->clear();
		if (m_convert.isNull()) 
			return;

		ui->filterDockWidget->setEnabled(true);
		ui->filterDockWidget->setWindowTitle(title);
		ui->paramsWidget->layout()->addWidget(m_convert->paramsWidget());
		
		_OnApply();
	});
	ui->menuProcess->addAction(action);
	ui->mainToolBar->addAction(action);
}

void MainWindow::_CreateProcessActions()
{
	_CreateProcessAction("gray", "Convert To Gray", []() { return new Gray(); });

	ui->menuProcess->addSeparator();
	ui->mainToolBar->addSeparator();

	_CreateProcessAction("threshold", "Threshold", []() { return new Threshold(); });
	_CreateProcessAction("adaptiveThreshold", "Adaptive Threshold", []() { return new AdaptiveThreshold(); });
	_CreateProcessAction("bilateralFilter", "Bilateral Filter", []() { return new BilateralFilter(); });
	_CreateProcessAction("blur", "Blur", []() { return new Blur(); });
	_CreateProcessAction("box", "Box Filter", []() { return new BoxFilter(); });
	_CreateProcessAction("gaussianBlur", "Gaussian Blur", []() { return new GaussianBlur(); });
	_CreateProcessAction("medianBlur", "Median Blur", []() { return new MedianBlur(); });
	_CreateProcessAction("canny", "Canny", []() { return new Canny(); });
	_CreateProcessAction("dilate", "Dilate", []() { return new Dilate(); });
	_CreateProcessAction("erode", "Erode", []() { return new Erode(); });

	ui->menuProcess->addSeparator();
	ui->mainToolBar->addSeparator();

	_CreateProcessAction("hough", "HoughCircles", []() { return new HoughCircles(); });
	_CreateProcessAction("fitEllipse", "FitEllipse", []() { return new FitEllipse(); });
}

void MainWindow::_Open(const QString &filePath)
{
	QImage image(filePath);
	if (image.isNull()) {
		m_recentFiles->remove(filePath);
		return;
	}

	m_recentFiles->add(filePath);
	ui->originalView->setImage(image);
	ui->originalView->setCurrentScale(0);//Auto scale
	ui->processView->clear();
	ui->processView->setCurrentScale(0);
	bool isGray = image.isGrayscale();
	m_originalMat = QtOcv::image2Mat(image, CV_8UC(isGray ? 1 : 3), QtOcv::MCO_RGB);
	setWindowTitle(QString("%1[*] - Image Process").arg(filePath));
}
