#include "window.h"

Window::Window()
{
    refresh_min=5;
    max_price=1000;
    min_price=100;

    setupUi();

    createActions();

    setIcon(0);
    trayIcon->show();

    setWindowTitle(tr("PrDell"));
    resize(400, 300);

    m_manager = new QNetworkAccessManager(this);
    m_manager->setCookieJar(new QNetworkCookieJar);
    connect(m_manager, SIGNAL(finished(QNetworkReply*)),
         this, SLOT(replyFinished(QNetworkReply*)));
    _qnam = createQNAM();
    dell_url="http://outlet.us.dell.com/ARBOnlineSales/Online/LikeConfigDetails.aspx?R=693&i0=59002DELL59003&fi0=9333DELL9491DELL4928&brandId=2801&CP=1&SC=lowToHigh&cnfgId=";
    urlEdit->setText(dell_url);

    timer_id=startTimer(60000*refresh_min);
}

void Window::setVisible(bool visible)
{
    refreshAction->setEnabled(true);
    QDialog::setVisible(visible);
}

void Window::timerEvent(QTimerEvent *event)
{
    if(event->timerId()==timer_id)
        refresh();
}

QNetworkAccessManager* Window::createQNAM() {
    QNetworkAccessManager* qnam = new QNetworkAccessManager(this);
    /* We'll handle the finished reply in replyFinished */
    connect(qnam, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));
    return qnam;
}

void Window::replyFinished(QNetworkReply* reply)
{
    QVariant possibleRedirectUrl =
             reply->attribute(QNetworkRequest::RedirectionTargetAttribute);

    _urlRedirectedTo = this->redirectUrl(possibleRedirectUrl.toUrl(),
                                         _urlRedirectedTo);
    if(!_urlRedirectedTo.isEmpty())
    {
        QString s="http://outlet.us.dell.com";
        s+=_urlRedirectedTo.toString();
        QUrl u(s);
        this->_qnam->get(QNetworkRequest(u));
    }
    else
    {
        QByteArray data=reply->readAll();
        QString str(data);
        parse(str);

        _urlRedirectedTo.clear();
    }

    reply->deleteLater();
}

QUrl Window::redirectUrl(const QUrl& possibleRedirectUrl,
                               const QUrl& oldRedirectUrl) const {
    QUrl redirectUrl;

    if(!possibleRedirectUrl.isEmpty() &&
       possibleRedirectUrl != oldRedirectUrl) {
        redirectUrl = possibleRedirectUrl;
    }
    return redirectUrl;
}

void Window::parse(const QString & str)
{
    QWebPage webPage;
    QWebFrame * frame = webPage.mainFrame();
    frame->setHtml(str);
    QWebElement doc = frame->documentElement();
    QWebElement body= doc.findFirst("body");
    QWebElementCollection price=doc.findAll("span[class=pricing_sale_price]");

    int tmp=100000;
    int id=-1;
    QRegExp rx("(\\d+)");
    for(int i=0;i<price.count();i++)
    {
        QString str=price.at(i).toPlainText();
        str=str.remove(',');

        int pos = 0;
        if ((pos = rx.indexIn(str, pos)) != -1)
        {
            QString n=str.mid(pos,rx.matchedLength());

            float t=n.toFloat();
            if(t>min_price && t<tmp)
            {
                tmp=t;
                id=i;
            }
        }
    }

    if(id!=-1 && tmp<max_price)
    {
        setIcon(1);
        ditchAction->setEnabled(true);
        showMessage("New viable laptop!!!", price.at(id).toPlainText());
        QWebElementCollection laptop=doc.findAll("td[class=para]");
        bodyEdit->setText(laptop.at(id).toPlainText());
    }
    else
    {
        if(str.length()==0)
            bodyEdit->setText("No response");
        else
        {
            QString low="Lowest price ";
            low+=QString::number(tmp);
            low+="USD \n";
            QWebElementCollection laptop=doc.findAll("td[class=para]");
            low+=laptop.at(id).toPlainText();
            bodyEdit->setText(low);
        }
    }
}

void Window::setIcon(int index)
{
    QIcon icon;
    if(index==0)
        icon=QIcon(":/images/trash.png");
    else
        icon=QIcon(":/images/heart.png");

    trayIcon->setIcon(icon);
    setWindowIcon(icon);
}

void Window::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
        open();
        break;
    default:;
    }
}

void Window::showMessage(const QString & title,const QString & text)
{
    trayIcon->showMessage(title, text, QSystemTrayIcon::Information, 1000);
}

void Window::changeRefresh(int value)
{
    refresh_min=value;
    killTimer(timer_id);
    timer_id=startTimer(value*60000);
}

void Window::refresh()
{
    QUrl url=dell_url;
    bodyEdit->setText("Pending...");
    m_manager->get(QNetworkRequest(url));
}

void Window::ditch()
{
    setIcon(0);
    ditchAction->setEnabled(false);
}

void Window::setMaxPrice(int price)
{
    max_price=price;
}

void Window::setMinPrice(int price)
{
    min_price=price;
}

void Window::setupUi()
{
    QGroupBox * messageGroupBox = new QGroupBox(tr("Setup"));

    QLabel * refreshLabel = new QLabel(tr("Refresh"));

    QSpinBox * spin=new QSpinBox;
    spin->setRange(0,1440);
    spin->setValue(refresh_min);
    spin->setSuffix("minute");
    connect(spin,SIGNAL(valueChanged(int)),this,SLOT(changeRefresh(int)));

    urlLabel = new QLabel("Dell outlet url");
    urlEdit = new QLineEdit(dell_url);
    bodyLabel = new QLabel("Laptops");

    bodyEdit = new QTextEdit;
    bodyEdit->setPlainText("");

    QLabel * pricelabel=new QLabel("Price USD");
    minPrice=new QSpinBox();
    minPrice->setSuffix("min");
    minPrice->setMaximum(10000);
    minPrice->setValue(min_price);
    connect(minPrice,SIGNAL(valueChanged(int)),this,SLOT(setMinPrice(int)));
    maxPrice=new QSpinBox();
    maxPrice->setSuffix("max");
    maxPrice->setMaximum(100000);
    maxPrice->setValue(max_price);
    connect(maxPrice,SIGNAL(valueChanged(int)),this,SLOT(setMaxPrice(int)));

    QGridLayout *messageLayout = new QGridLayout;
    messageLayout->addWidget(refreshLabel, 0, 0);
    messageLayout->addWidget(spin, 0, 1, 1, 2);
    messageLayout->addWidget(pricelabel,1,0);
    messageLayout->addWidget(minPrice,1,1);
    messageLayout->addWidget(maxPrice,1,2);
    messageLayout->addWidget(urlLabel, 2, 0);
    messageLayout->addWidget(urlEdit, 2, 1, 1, 4);
    messageLayout->addWidget(bodyLabel, 3, 0);
    messageLayout->addWidget(bodyEdit, 3, 1, 2, 4);
    messageLayout->setColumnStretch(3, 1);
    messageLayout->setRowStretch(4, 1);
    messageGroupBox->setLayout(messageLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(messageGroupBox);
    setLayout(mainLayout);

    QHBoxLayout * layout=new QHBoxLayout;
    mainLayout->addLayout(layout);
    QPushButton *

    button = new QPushButton(tr("Refresh"));
    connect(button, SIGNAL(clicked()), this, SLOT(refresh()));
    layout->addWidget(button);

    button = new QPushButton(tr("Close"));
    button->setDefault(true);
    connect(button, SIGNAL(clicked()), this, SLOT(close()));
    layout->addWidget(button);

    button = new QPushButton(tr("Quit"));
    connect(button, SIGNAL(clicked()), qApp, SLOT(quit()));
    layout->addWidget(button);
}

void Window::createActions()
{
    ditchAction = new QAction(tr("&Ditch"), this);
    ditchAction->setEnabled(false);
    connect(ditchAction, SIGNAL(triggered()), this, SLOT(ditch()));

    refreshAction = new QAction(tr("&Refresh"), this);
    connect(refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));

    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(ditchAction);
    trayIconMenu->addAction(refreshAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
}
