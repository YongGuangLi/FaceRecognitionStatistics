#include "simpleasyncconsumer.h"

SimpleAsyncConsumer::SimpleAsyncConsumer(const std::string& brokerURI,
                    const std::string& destURI,
                    bool useTopic,
                    bool clientAck) :
    connection(NULL),
    session(NULL),
    destination(NULL),
    consumer(NULL),
    useTopic(useTopic),
    brokerURI(brokerURI),
    destURI(destURI),
    clientAck(clientAck) {
	qRegisterMetaType<CommEventLog>("CommEventLog");
}

SimpleAsyncConsumer::~SimpleAsyncConsumer()
{
    this->cleanup();
}

void SimpleAsyncConsumer::close()
{
    this->cleanup();
}

void SimpleAsyncConsumer::runConsumer()
{
    try {

        // Create a ConnectionFactory
        ActiveMQConnectionFactory* connectionFactory = new ActiveMQConnectionFactory(brokerURI);

        // Create a Connection
        connection = connectionFactory->createConnection();
        delete connectionFactory;

        ActiveMQConnection* amqConnection = dynamic_cast<ActiveMQConnection*>(connection);
        if (amqConnection != NULL) {
            amqConnection->addTransportListener(this);
        }

        connection->start();

        connection->setExceptionListener(this);

        // Create a Session
        if (clientAck) {
            session = connection->createSession(Session::CLIENT_ACKNOWLEDGE);
        } else {
            session = connection->createSession(Session::AUTO_ACKNOWLEDGE);
        }

        // Create the destination (Topic or Queue)
        if (useTopic) {
            destination = session->createTopic(destURI);
        } else {
            destination = session->createQueue(destURI);
        }

        // Create a MessageConsumer from the Session to the Topic or Queue
        consumer = session->createConsumer(destination);
        consumer->setMessageListener(this);

    } catch (CMSException& e) {
        e.printStackTrace();
    }
}

void SimpleAsyncConsumer::onMessage(const Message *message)
{
    try {
        const BytesMessage* bytesMessage = dynamic_cast<const BytesMessage*>(message);
        if (bytesMessage != NULL) {   
			unsigned char* bodyBytes = bytesMessage->getBodyBytes(); 
			CommEventLog commEventLog;
			if(commEventLog.ParseFromArray(bodyBytes, bytesMessage->getBodyLength())) 
				emit sendMessage(commEventLog); 

			delete bodyBytes;
			bodyBytes = NULL;
        }

        if (clientAck) {
            message->acknowledge();
        }

    } catch (CMSException& e) {
        e.printStackTrace();
    }
}

void SimpleAsyncConsumer::onException(const CMSException &ex)
{
    printf("CMS Exception occurred.  Shutting down client.\n");
    exit(1);
}

void SimpleAsyncConsumer::onException(const Exception &ex)
{
    printf("Transport Exception occurred: %s \n", ex.getMessage().c_str());
}

void SimpleAsyncConsumer::transportInterrupted()
{
    std::cout << "The Connection's Transport has been Interrupted." << std::endl;
}

void SimpleAsyncConsumer::transportResumed()
{
    std::cout << "The Connection's Transport has been Restored." << std::endl;
}

void SimpleAsyncConsumer::cleanup()
{
    //*************************************************
    // Always close destination, consumers and producers before
    // you destroy their sessions and connection.
    //*************************************************

    // Destroy resources.
    try{
        if( destination != NULL ) 
			delete destination;
    }catch (CMSException& e) 
	{}
    destination = NULL;

    try{
        if( consumer != NULL ) 
			delete consumer;
    }catch (CMSException& e) 
	{}
    consumer = NULL;

    // Close open resources.
    try{
        if( session != NULL ) 
			session->close();
        if( connection != NULL ) 
			connection->close();
    }catch (CMSException& e) {}

    // Now Destroy them
    try{
        if( session != NULL ) 
			delete session;
    }catch (CMSException& e) 
	{}
    session = NULL;

    try{
        if( connection != NULL ) 
			delete connection;
    }catch (CMSException& e) 
	{}
    connection = NULL;
}
