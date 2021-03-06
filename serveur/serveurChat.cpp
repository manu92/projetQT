// Importatins
#include "serveurChat.h"
#include <QTcpSocket>
#include <QRegExp>

// Constructeur sans argument
ChatServeur::ChatServeur(QObject *parent) : QTcpServer(parent) { }

// Méthode de réception des messages et aiguillage (message/déconnexion)
void ChatServeur::incomingConnection(int socketfd) {
    // Creation d'une socket TCP
    QTcpSocket *client = new QTcpSocket(this);
    client->setSocketDescriptor(socketfd);
    clients.insert(client);

    qDebug() << "Nouveau Client:";

    // Creation du slot de lecture
    connect(client, SIGNAL(readyRead()), this, SLOT(readyRead()));

    // Creation du slot de deconnexion
    connect(client, SIGNAL(disconnected()), this, SLOT(disconnected()));
}

// Réception des messages
void ChatServeur::readyRead() {
    // Récupération de la socket TCP du client
    QTcpSocket *client = (QTcpSocket*)sender();

    // Boucle de lecture
    while(client->canReadLine()) {
        // Transformation du message en QString
	// trimmed() enlève les espaces en trop
        QString line = QString::fromUtf8(client->readLine()).trimmed();

	qDebug() << "Nouvelle ligne:" << line;

	// Si le texte répond à la regex "/moi..../image" => connexion d'un client
        QRegExp meRegex("^/moi:(.*)/image:(.*)$");

	// Si connexion
        if(meRegex.indexIn(line) != -1) {
	    // Récupération de l'utilisateur
            QString user = meRegex.cap(1);
            users[client] = user;

	    // Récupération de l'image de l'utilisateur
	    user_images[user] = meRegex.cap(2);

	    // On envoi à tous utilisateurs la connexon du client 
            foreach(QTcpSocket *client, clients)
                client->write(QString("Serveur:" + user + " a rejoint le chat.\n").toUtf8());
            sendUserList();
        } else 
        // Si pas connexion
	if(users.contains(client)) {
	    // on réupère le message et l'utilisateur
            QString message = line;
            QString user = users[client];

	    qDebug() << "Utilisateur:" << user;
            qDebug() << "Message:" << message;

	    // On envoi à tous utilisateurs le message
            foreach(QTcpSocket *otherClient, clients)
                otherClient->write(QString(user + ":" + message + "\n").toUtf8());
        } else {
	    // Si erreur
            qWarning() << "Erreur message client:" << client->peerAddress().toString() << line;
        }
    }
}

// Méthode de deconnexion d'un utilisateur
void ChatServeur::disconnected() {
    // on récupère le client dans notre liste pour le supprimer
    QTcpSocket *client = (QTcpSocket*)sender();
    clients.remove(client);

    qDebug() << "Client déconnecté";

    // on récupère l'utilisateur dans notre liste pour le supprimer
    QString user = users[client];
    users.remove(client);

    // On envoi à tous les utilisateurs la liste des utilisateurs
    sendUserList();

    // On envoi à tous les utilisateurs la déconnexion du client
    foreach(QTcpSocket *client, clients)
        client->write(QString("Serveur:" + user + " a quitté le chat.\n").toUtf8());
}

// Méhode d'envoi à la liste des utilisateurs
void ChatServeur::sendUserList() {
    // Mise à jour des clients connectés
    QStringList userList;
    foreach(QString user, users.values())
        userList << user;

    // Mise à jour des images des clients connectés
    QStringList images;
    foreach(QString image, user_images.values())
	images << image;

    // Mise à jour de la liste des utilisateurs et leurs images
    foreach(QTcpSocket *client, clients)
        client->write(QString("/users:" + userList.join(",") + "/images:" + images.join(",") + "\n").toUtf8());
}
