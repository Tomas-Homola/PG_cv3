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
	if (polygonPoints.size() == 2) // usecka
		createLineWithAlgorithm(polygonPoints.at(0), polygonPoints.at(1), color, ui->comboBox_SelectAlgorithm->currentIndex());
	else if (polygonPoints.size() > 2) // polygon
	{
		for (int i = 1; i <= polygonPoints.size(); i++)
		{
			if (i == polygonPoints.size())
				createLineWithAlgorithm(polygonPoints.at(0), polygonPoints.at(i - 1), color, ui->comboBox_SelectAlgorithm->currentIndex());
			else
				createLineWithAlgorithm(polygonPoints.at(i), polygonPoints.at(i - 1), color, ui->comboBox_SelectAlgorithm->currentIndex());
		}
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
	//qDebug() << "line drawn:" << point1 << point2;
	if (algorithm == 0) // DDA
		getCurrentViewerWidget()->drawLineDDA(point1, point2, color);
	else if (algorithm == 1) // mr. Bresenham
		getCurrentViewerWidget()->drawLineBresenham(point1, point2, color);
}

void ImageViewer::trimLine(QVector<QPoint>& currentLine)
{
	int imgHeight = getCurrentViewerWidget()->getImgHeight();
	int imgWidth = getCurrentViewerWidget()->getImgWidth();
	QPoint E[4]; // pole vrcholov obrazka
	E[0] = QPoint(0, 0); E[3] = QPoint(imgWidth, 0);
	E[1] = QPoint(0, imgHeight); E[2] = QPoint(imgWidth, imgHeight);
	QPoint P1 = currentLine[0], P2 = currentLine[1];
	QPoint newP1(0, 0), newP2(0, 0);
	QVector<QPoint> newLine;
	QPoint vectorD = P2 - P1, vectorW(0, 0), vectorE(0, 0), normalE(0, 0);
	double tL = 0.0, tU = 1.0, t = 0.0;
	int dotProductDN = 0, dotProductWN = 0;

	bool areInside = false;
	bool shouldTrim = false;

	if ((P1.x() >= 0 && P1.x() <= imgWidth && P1.y() >= 0 && P1.y() <= imgHeight) || (P2.x() >= 0 && P2.x() <= imgWidth && P2.y() >= 0 && P2.y() <= imgHeight))
		areInside = true;

	//https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
	if (!areInside) // ci sa usecka nachadza v nejakom rohu tak, ze oba body su uz mimo, ale mala by sa este osekavat
	{
		int intersections = 0;
		double s = 0.0, t = 0.0;
		int denom = 0;
		int upperS = 0;
		int upperT = 0;

		for (int i = 0; i < 4; i++)
		{
			if (i == 3)
				vectorE = E[0] - E[3];
			else
				vectorE = E[i + 1] - E[i];

			denom = -vectorE.x() * vectorD.y() + vectorD.x() * vectorE.y();
			upperS = -vectorD.y() * (P1.x() - E[i].x()) + vectorD.x() * (P1.y() - E[i].y());
			upperT = vectorE.x() * (P1.y() - E[i].y()) - vectorE.y() * (P1.x() - E[i].x());
			s = static_cast<double>(upperS) / denom;
			t = static_cast<double>(upperT) / denom;

			if (s >= 0.0 && s <= 1.0 && t >= 0 && t <= 1.0)
				intersections++;
		}

		if (intersections != 2)
			shouldTrim = false;
		else
			shouldTrim = true;
	}

	if (areInside || shouldTrim)
	{
		qDebug() << "trimming line";

		for (int i = 0; i < 4; i++)
		{
			// direct vector hrany obrazka
			if (i == 3)
				vectorE = E[0] - E[3];
			else
				vectorE = E[i + 1] - E[i];

			// z direct vectora spravime normalu: (x, y) -> (y, -x)
			normalE.setX(vectorE.y()); normalE.setY(-vectorE.x());
			vectorW = P1 - E[i];

			// skalarne suciny
			dotProductDN = QPoint::dotProduct(vectorD, normalE);
			dotProductWN = QPoint::dotProduct(vectorW, normalE);

			if (dotProductDN != 0)
			{
				t = static_cast<double>(-dotProductWN) / dotProductDN;

				if (dotProductDN > 0 && t <= 1.0)
					tL = std::max(t, tL);
				else if (dotProductDN < 0 && t >= 0.0)
					tU = std::min(t, tU);
			}
		}
		qDebug() << "tL:" << tL << "\ttU:" << tU;
		if (tL == 0.0 && tU == 1.0)
			drawPolygon(currentLine, currentColor);
		else if (tL < tU)
		{
			newP1.setX(static_cast<int>(P1.x() + ((double)P2.x() - P1.x()) * tL));
			newP1.setY(static_cast<int>(P1.y() + ((double)P2.y() - P1.y()) * tL));

			newP2.setX(static_cast<int>(P1.x() + ((double)P2.x() - P1.x()) * tU));
			newP2.setY(static_cast<int>(P1.y() + ((double)P2.y() - P1.y()) * tU));

			newLine.push_back(newP1); newLine.push_back(newP2);

			drawPolygon(newLine, currentColor);
		}
	}
	else
		drawPolygon(polygonPoints, currentColor);
}

void ImageViewer::trimPolygon(QVector<QPoint>& polygonPoints)
{
	QVector<QPoint> V = polygonPoints; // kopia bodov
	//printPoints(V);
	QVector<QPoint> W;
	QPoint S(0, 0);
	int xMin[4] = { 0,0, -getCurrentViewerWidget()->getImgWidth() + 1, -getCurrentViewerWidget()->getImgHeight() + 1 }; // poznamka pre autora: obrazok v poznamkach ku tymto hodnotam; z nejakeho dovodu ak tam nie je +1, to nekresli na spodnu hranu obrazka
	int temp = 0;

	for (int i = 0; i < 4; i++)
	{
		if (V.size() != 0)
			S = V[V.size() - 1];

		for (int j = 0; j < V.size(); j++)
		{
			if (V.at(j).x() >= xMin[i])
			{
				if (S.x() >= xMin[i])
					W.push_back(V[j]);
				else
				{
					temp = static_cast<int>(S.y() + ((double)xMin[i] - S.x()) * ((double)V[j].y() - S.y()) / ((double)V[j].x() - S.x()) + 0.5);
					//temp = static_cast<double>(S.y()) + (static_cast<double>(xMin[i]) - S.x()) * static_cast<double>(((V.at(j).y() - S.y()) / (V.at(j).x() - S.x())));
					W.push_back(QPoint(xMin[i], temp)); // priesecnik P
					W.push_back(V[j]);
				}
			}
			else
			{
				if (S.x() >= xMin[i])
				{
					temp = static_cast<int>(S.y() + ((double)xMin[i] - S.x()) * ((double)V[j].y() - S.y()) / ((double)V[j].x() - S.x()) + 0.5);
					//temp = static_cast<double>(S.y()) + (static_cast<double>(xMin[i]) - S.x()) * static_cast<double>(((V.at(j).y() - S.y()) / (V.at(j).x() - S.x())));
					W.push_back(QPoint(xMin[i], temp)); // priesecnik P
				}
			}

			S = V.at(j);
		}

		V.clear();

		for (int k = 0; k < W.size(); k++)
			V.push_back(QPoint(W[k].y(), -W[k].x()));

		W.clear();
	}
	printPoints(V);
	drawPolygon(V, currentColor);
}

void ImageViewer::trim(QVector<QPoint>& polygonPoints)
{
	if (polygonPoints.size() == 2)
		trimLine(polygonPoints);
	else if (polygonPoints.size() > 2)
		trimPolygon(polygonPoints);
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
		if (mousePosition[1] != e->pos())
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

			trim(polygonPoints);
			//drawPolygon(polygonPoints, currentColor);
		}
		
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

		trim(polygonPoints);
		//drawPolygon(polygonPoints, currentColor);

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

		trim(polygonPoints);
		//drawPolygon(polygonPoints, currentColor);
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
		
		trim(polygonPoints);
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

	trim(polygonPoints);
	//drawPolygon(polygonPoints, currentColor);
}

void ImageViewer::on_pushButton_Shear_clicked()
{
	double shearFactor = ui->doubleSpinBox_ShearFactor->value();
	double sY = polygonPoints.at(0).y();

	for (int i = 1; i < polygonPoints.size(); i++)
		polygonPoints[i].setX(static_cast<int>(polygonPoints.at(i).x() + shearFactor * (polygonPoints.at(i).y() - sY)));

	getCurrentViewerWidget()->clear();

	trim(polygonPoints);
	//drawPolygon(polygonPoints, currentColor);
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

	trim(polygonPoints);
	//drawPolygon(polygonPoints, currentColor);
}

