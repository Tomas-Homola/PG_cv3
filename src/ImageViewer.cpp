#include "ImageViewer.h"

ImageViewer::ImageViewer(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::ImageViewerClass)
{
	ui->setupUi(this);

	ui->pushButton_ColorDialog->setStyleSheet("background-color:#000000");
	currentColor = QColor("#000000");

	openNewTabForImg(new ViewerWidget("Default window", QSize(800, 450)));
	ui->tabWidget->setCurrentIndex(ui->tabWidget->count() - 1);

	ui->pushButton_ClearPolygon->setEnabled(false);
	ui->groupBox_Transformations->setEnabled(false);
}

void ImageViewer::infoMessage(QString message)
{
	msgBox.setWindowTitle("Info message");
	msgBox.setIcon(QMessageBox::Information);
	msgBox.setText(message);
	msgBox.exec();
}

void ImageViewer::warningMessage(QString message)
{
	msgBox.setWindowTitle("Warning message");
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setText(message);
	msgBox.exec();
}

void ImageViewer::drawPolygon(QVector<QPoint>& polygonPoints, QColor color)
{
	for (int i = 1; i <= polygonPoints.size(); i++)
	{
		if (i == polygonPoints.size())
			createLineWithAlgorithm(polygonPoints.at(0), polygonPoints.at(i - 1), QColor("#ff0000"), ui->comboBox_SelectAlgorithm->currentIndex());
		else
			createLineWithAlgorithm(polygonPoints.at(i), polygonPoints.at(i - 1), QColor("#ff0000"), ui->comboBox_SelectAlgorithm->currentIndex());
	}
}

void ImageViewer::printPoints(QVector<QPoint>& polygonPoints)
{
	for (int i = 0; i < polygonPoints.size(); i++)
		qDebug() << polygonPoints.at(i);
	qDebug() << "\n";
}

void ImageViewer::createLineWithAlgorithm(QPoint point1, QPoint point2, QColor color, int algorithm)
{
	if (algorithm == 0) // DDA
		getCurrentViewerWidget()->drawLineDDA(point1, point2, color);
	else if (algorithm == 1) // mr. Bresenham
		getCurrentViewerWidget()->drawLineBresenham(point1, point2, color);
}

//ViewerWidget functions
ViewerWidget* ImageViewer::getViewerWidget(int tabId)
{
	QScrollArea* s = static_cast<QScrollArea*>(ui->tabWidget->widget(tabId));
	if (s) {
		ViewerWidget* vW = static_cast<ViewerWidget*>(s->widget());
		return vW;
	}
	return nullptr;
}
ViewerWidget* ImageViewer::getCurrentViewerWidget()
{
	return getViewerWidget(ui->tabWidget->currentIndex());
}

// Event filters
bool ImageViewer::eventFilter(QObject* obj, QEvent* event)
{
	if (obj->objectName() == "ViewerWidget") {
		return ViewerWidgetEventFilter(obj, event);
	}
	return false;
}

//ViewerWidget Events
bool ImageViewer::ViewerWidgetEventFilter(QObject* obj, QEvent* event)
{
	ViewerWidget* w = static_cast<ViewerWidget*>(obj);

	if (!w) {
		return false;
	}

	if (event->type() == QEvent::MouseButtonPress) {
		ViewerWidgetMouseButtonPress(w, event);
	}
	else if (event->type() == QEvent::MouseButtonRelease) {
		ViewerWidgetMouseButtonRelease(w, event);
	}
	else if (event->type() == QEvent::MouseMove) {
		ViewerWidgetMouseMove(w, event);
	}
	else if (event->type() == QEvent::Leave) {
		ViewerWidgetLeave(w, event);
	}
	else if (event->type() == QEvent::Enter) {
		ViewerWidgetEnter(w, event);
	}
	else if (event->type() == QEvent::Wheel) {
		ViewerWidgetWheel(w, event);
	}

	return QObject::eventFilter(obj, event);
}
void ImageViewer::ViewerWidgetMouseButtonPress(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);
	if (drawingEnabled) // ide sa kreslit
	{
		if (e->button() == Qt::LeftButton)
		{
			polygonPoints.push_back(e->pos());
			if (polygonPoints.size() > 1)
			{
				createLineWithAlgorithm(polygonPoints.at(polygonPoints.size() - 1), polygonPoints.at(polygonPoints.size() - 2), currentColor, ui->comboBox_SelectAlgorithm->currentIndex());
			}
			printPoints(polygonPoints);

		}
		else if (e->button() == Qt::RightButton) // ukoncenie kreslenia
		{
			if (polygonPoints.size() == 1) // kliknutie pravym hned po zadani prveho bodu
			{
				polygonPoints.push_back(e->pos());
				createLineWithAlgorithm(polygonPoints.at(1), polygonPoints.at(0), currentColor, ui->comboBox_SelectAlgorithm->currentIndex());
			}
			else if (polygonPoints.size() > 2) // ak by uz bola nakreslena usecka, tak sa znovu nenakresli
			{
				createLineWithAlgorithm(polygonPoints.at(polygonPoints.size() - 1), polygonPoints.at(0), currentColor, ui->comboBox_SelectAlgorithm->currentIndex());
			}
			
			drawingEnabled = false;
			ui->groupBox_Transformations->setEnabled(true);

			printPoints(polygonPoints);
		}
	}
	else // nejde sa kreslit, ale posuvat polygon
	{
		mousePosition[0] = e->pos();
	}
		
}
void ImageViewer::ViewerWidgetMouseButtonRelease(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);

	if (e->button() == Qt::LeftButton && !drawingEnabled)
	{
		mousePosition[1] = e->pos();

		int pX = mousePosition[1].x() - mousePosition[0].x();
		int pY = mousePosition[1].y() - mousePosition[0].y();

		for (int i = 0; i < polygonPoints.size(); i++) // prepocitanie suradnic polygonu
		{
			// poznamka pre autora: [i] vracia modifikovatelny objekt, .at(i) vracia const objekt
			polygonPoints[i].setX(polygonPoints[i].x() + pX);
			polygonPoints[i].setY(polygonPoints[i].y() + pY);
		}

		getCurrentViewerWidget()->clear(); // vymazanie stareho polygonu

		drawPolygon(polygonPoints, currentColor);
	}
}
void ImageViewer::ViewerWidgetMouseMove(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);

	if (e->buttons() == Qt::LeftButton && !drawingEnabled)
	{
		qDebug() << "mouse moved";
		mousePosition[1] = e->pos();
		int pX = mousePosition[1].x() - mousePosition[0].x();
		int pY = mousePosition[1].y() - mousePosition[0].y();
		
		for (int i = 0; i < polygonPoints.size(); i++)
		{
			polygonPoints[i].setX(polygonPoints[i].x() + pX);
			polygonPoints[i].setY(polygonPoints[i].y() + pY);
		}

		getCurrentViewerWidget()->clear(); // vymazanie stareho polygonu

		drawPolygon(polygonPoints, currentColor);

		mousePosition[0] = mousePosition[1];
	}
}
void ImageViewer::ViewerWidgetLeave(ViewerWidget* w, QEvent* event)
{
}
void ImageViewer::ViewerWidgetEnter(ViewerWidget* w, QEvent* event)
{
}
void ImageViewer::ViewerWidgetWheel(ViewerWidget* w, QEvent* event)
{
	QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);

	if (!drawingEnabled)
	{
		double scaleFactorXY = 0.0;
		double sX = polygonPoints.at(0).x();
		double sY = polygonPoints.at(0).y();


		if (wheelEvent->angleDelta().y() > 0)
			scaleFactorXY = 1.25;
		else if (wheelEvent->angleDelta().y() < 0)
			scaleFactorXY = 0.75;

		for (int i = 0; i < polygonPoints.size(); i++)
		{
			polygonPoints[i].setX(sX + static_cast<int>((polygonPoints.at(i).x() - sX) * scaleFactorXY));
			polygonPoints[i].setY(sY + static_cast<int>((polygonPoints.at(i).y() - sY) * scaleFactorXY));
		}

		getCurrentViewerWidget()->clear();

		drawPolygon(polygonPoints, currentColor);
	}
}

//ImageViewer Events
void ImageViewer::closeEvent(QCloseEvent* event)
{
	if (QMessageBox::Yes == QMessageBox::question(this, "Close Confirmation", "Are you sure you want to exit?", QMessageBox::Yes | QMessageBox::No))
	{
		event->accept();
	}
	else {
		event->ignore();
	}
}

//Image functions
void ImageViewer::openNewTabForImg(ViewerWidget* vW)
{
	QScrollArea* scrollArea = new QScrollArea;
	scrollArea->setWidget(vW);

	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	vW->setObjectName("ViewerWidget");
	vW->installEventFilter(this);

	QString name = vW->getName();

	ui->tabWidget->addTab(scrollArea, name);
}
bool ImageViewer::openImage(QString filename)
{
	QFileInfo fi(filename);

	QString name = fi.baseName();
	openNewTabForImg(new ViewerWidget(name, QSize(0, 0)));
	ui->tabWidget->setCurrentIndex(ui->tabWidget->count() - 1);

	ViewerWidget* w = getCurrentViewerWidget();

	QImage loadedImg(filename);
	return w->setImage(loadedImg);
}
bool ImageViewer::saveImage(QString filename)
{
	QFileInfo fi(filename);
	QString extension = fi.completeSuffix();
	ViewerWidget* w = getCurrentViewerWidget();

	QImage* img = w->getImage();
	return img->save(filename, extension.toStdString().c_str());
}
void ImageViewer::clearImage()
{
	ViewerWidget* w = getCurrentViewerWidget();
	w->clear();
}
void ImageViewer::setBackgroundColor(QColor color)
{
	ViewerWidget* w = getCurrentViewerWidget();
	if (w != nullptr)
		w->clear(color);
	else
		warningMessage("No image opened");
}

//Slots

//Tabs slots
void ImageViewer::on_tabWidget_tabCloseRequested(int tabId)
{
	ViewerWidget* vW = getViewerWidget(tabId);
	delete vW; //vW->~ViewerWidget();
	ui->tabWidget->removeTab(tabId);
}
void ImageViewer::on_actionRename_triggered()
{
	if (!isImgOpened()) {
		msgBox.setText("No image is opened.");
		msgBox.setIcon(QMessageBox::Information);
		msgBox.exec();
		return;
	}
	ViewerWidget* w = getCurrentViewerWidget();
	bool ok;
	QString text = QInputDialog::getText(this, QString("Rename image"), tr("Image name:"), QLineEdit::Normal, w->getName(), &ok);
	if (ok && !text.trimmed().isEmpty())
	{
		w->setName(text);
		ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), text);
	}
}

//Image slots
void ImageViewer::on_actionNew_triggered()
{
	newImgDialog = new NewImageDialog(this);
	connect(newImgDialog, SIGNAL(accepted()), this, SLOT(newImageAccepted()));
	newImgDialog->exec();
}
void ImageViewer::newImageAccepted()
{
	NewImageDialog* newImgDialog = static_cast<NewImageDialog*>(sender());

	int width = newImgDialog->getWidth();
	int height = newImgDialog->getHeight();
	QString name = newImgDialog->getName();
	openNewTabForImg(new ViewerWidget(name, QSize(width, height)));
	ui->tabWidget->setCurrentIndex(ui->tabWidget->count() - 1);
}
void ImageViewer::on_actionOpen_triggered()
{
	QString folder = settings.value("folder_img_load_path", "").toString();

	QString fileFilter = "Image data (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm .*xbm .* xpm);;All files (*)";
	QString fileName = QFileDialog::getOpenFileName(this, "Load image", folder, fileFilter);
	if (fileName.isEmpty()) { return; }

	QFileInfo fi(fileName);
	settings.setValue("folder_img_load_path", fi.absoluteDir().absolutePath());

	if (!openImage(fileName)) {
		msgBox.setText("Unable to open image.");
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.exec();
	}
}
void ImageViewer::on_actionSave_as_triggered()
{
	if (!isImgOpened()) {
		msgBox.setText("No image to save.");
		msgBox.setIcon(QMessageBox::Information);
		msgBox.exec();
		return;
	}
	QString folder = settings.value("folder_img_save_path", "").toString();

	ViewerWidget* w = getCurrentViewerWidget();

	QString fileFilter = "Image data (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm .*xbm .* xpm);;All files (*)";
	QString fileName = QFileDialog::getSaveFileName(this, "Save image", folder + "/" + w->getName(), fileFilter);
	if (fileName.isEmpty()) { return; }

	QFileInfo fi(fileName);
	settings.setValue("folder_img_save_path", fi.absoluteDir().absolutePath());

	if (!saveImage(fileName)) {
		msgBox.setText("Unable to save image.");
		msgBox.setIcon(QMessageBox::Warning);
		msgBox.exec();
	}
	else {
		msgBox.setText(QString("File %1 saved.").arg(fileName));
		msgBox.setIcon(QMessageBox::Information);
		msgBox.exec();
	}
}
void ImageViewer::on_actionClear_triggered()
{
	if (!isImgOpened()) {
		msgBox.setText("No image is opened.");
		msgBox.setIcon(QMessageBox::Information);
		msgBox.exec();
		return;
	}
	clearImage();
}
void ImageViewer::on_actionSet_background_color_triggered()
{
	QColor backgroundColor = QColorDialog::getColor(Qt::white, this, "Select color of background");
	if (backgroundColor.isValid()) {
		setBackgroundColor(backgroundColor);
	}
}

void ImageViewer::on_pushButton_ColorDialog_clicked()
{
	QColor chosenColor = QColorDialog::getColor(currentColor.name(), this, "Select pen color");

	if (chosenColor.isValid())
	{
		currentColor = chosenColor;
		ui->pushButton_ColorDialog->setStyleSheet(QString("background-color:%1").arg(chosenColor.name()));
	}
}

void ImageViewer::on_pushButton_CreatePolygon_clicked()
{
	ui->pushButton_CreatePolygon->setEnabled(false);
	ui->pushButton_ClearPolygon->setEnabled(true);

	drawingEnabled = true;
}

void ImageViewer::on_pushButton_ClearPolygon_clicked()
{
	ui->pushButton_ClearPolygon->setEnabled(false);
	ui->pushButton_CreatePolygon->setEnabled(true);
	ui->groupBox_Transformations->setEnabled(false);

	drawingEnabled = false;
	//qDebug() << "polygon size before:" << polygonPoints.size();
	polygonPoints.clear();
	//qDebug() << "polygon size after:" << polygonPoints.size();
	getCurrentViewerWidget()->clear();
}

void ImageViewer::on_pushButton_Rotate_clicked()
{
	double angle = (ui->spinBox_Angle->value() / 180.0) * M_PI;
	double sX = polygonPoints.at(0).x();
	double sY = polygonPoints.at(0).y();
	double x = 0.0, y = 0.0;

	if (ui->spinBox_Angle->value() < 0)
	{
		//qDebug() << "clockwise";
		
		for (int i = 1; i < polygonPoints.size(); i++)
		{
			x = polygonPoints.at(i).x();
			y = polygonPoints.at(i).y();

			polygonPoints[i].setX(static_cast<int>((x - sX) * qCos(angle) + (y - sY) * qSin(angle) + sX));
			polygonPoints[i].setY(static_cast<int>(-(x - sX) * qSin(angle) + (y - sY) * qCos(angle) + sY));
		}
	}
	else if (ui->spinBox_Angle->value() > 0)
	{
		//qDebug() << "anti-clockwise";
		angle = 2 * M_PI - angle;
		for (int i = 1; i < polygonPoints.size(); i++)
		{
			x = polygonPoints.at(i).x();
			y = polygonPoints.at(i).y();

			polygonPoints[i].setX(static_cast<int>((x - sX) * qCos(angle) - (y - sY) * qSin(angle) + sX));
			polygonPoints[i].setY(static_cast<int>((x - sX) * qSin(angle) + (y - sY) * qCos(angle) + sY));
		}
	}

	getCurrentViewerWidget()->clear();

	drawPolygon(polygonPoints, currentColor);
}

void ImageViewer::on_pushButton_Shear_clicked()
{
	double shearFactor = ui->doubleSpinBox_ShearFactor->value();
	double sY = polygonPoints.at(0).y();

	for (int i = 1; i < polygonPoints.size(); i++)
		polygonPoints[i].setX(static_cast<int>(polygonPoints.at(i).x() + shearFactor * (polygonPoints.at(i).y() - sY)));

	getCurrentViewerWidget()->clear();

	drawPolygon(polygonPoints, currentColor);
}

void ImageViewer::on_pushButton_Symmetry_clicked()
{
	// symetria polygonu cez usecku medzi prvym a druhym bodom
	// symetria usecky cez horizontalnu priamku prechadzajucu stredom usecky
	double u = static_cast<double>(polygonPoints.at(1).x()) - polygonPoints.at(0).x();
	double v = static_cast<double>(polygonPoints.at(1).y()) - polygonPoints.at(0).y();
	double a = v;
	double b = -u;
	double c = -a * polygonPoints.at(0).x() - b * polygonPoints.at(0).y();
	double x = 0.0, y = 0.0;
	int midPointX = qAbs((polygonPoints.at(1).x() + polygonPoints.at(0).x()) / 2);
	int midPointY = qAbs((polygonPoints.at(1).y() + polygonPoints.at(0).y()) / 2);
	int deltaY = 0;

	if (polygonPoints.size() == 2) // usecka
	{
		deltaY = qAbs(polygonPoints.at(0).y() - midPointY); // 

		if (polygonPoints.at(0).y() < midPointY)
		{
			polygonPoints[0].setY(polygonPoints.at(0).y() + 2 * deltaY);
			polygonPoints[1].setY(polygonPoints.at(1).y() - 2 * deltaY);
		}
		else if (polygonPoints.at(0).y() > midPointY)
		{
			polygonPoints[0].setY(polygonPoints.at(0).y() - 2 * deltaY);
			polygonPoints[1].setY(polygonPoints.at(1).y() + 2 * deltaY);
		}
	}
	else if (polygonPoints.size() > 2) // polygon
	{
		for (int i = 2; i < polygonPoints.size(); i++)
		{
			x = polygonPoints.at(i).x();
			y = polygonPoints.at(i).y();

			polygonPoints[i].setX(static_cast<int>(x - 2 * a * ((a * x + b * y + c) / (a * a + b * b))));
			polygonPoints[i].setY(static_cast<int>(y - 2 * b * ((a * x + b * y + c) / (a * a + b * b))));
		}
	}

	getCurrentViewerWidget()->clear();

	drawPolygon(polygonPoints, currentColor);
}

