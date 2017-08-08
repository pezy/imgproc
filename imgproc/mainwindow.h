#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QSharedPointer>
#include "cvmatandqimage.h"

namespace Ui {
	class MainWindow;
}
class RecentFiles;
class AbstractConvert;
class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
	void closeEvent(QCloseEvent *evt) override;
	
	void _LoadSettings();
	void _SaveSettings();
	void _OnApply();
	void _CreateProcessAction(const QString& iconName, const QString& title, std::function<AbstractConvert*()> functor);
	void _CreateProcessActions();
	void _Open(const QString &filePath);

	Ui::MainWindow *ui;
	RecentFiles *m_recentFiles;
	cv::Mat m_originalMat;
	cv::Mat m_processMat;
	QSharedPointer<AbstractConvert> m_convert;
};

#endif // MAINWINDOW_H
