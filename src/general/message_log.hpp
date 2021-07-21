/**
 * @file message_log.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MESSAGE_LOG_HPP__
#define MESSAGE_LOG_HPP__

#include "opencmiss/zinc/types/loggerid.h"
#include "opencmiss/zinc/logger.h"
#include "opencmiss/zinc/status.h"
#include "general/debug.h"
#include "general/message.h"
#include "general/mystring.h"
#include <deque>
#include <list>
#include <string>
#include <utility>

struct cmzn_logger;

typedef std::pair<cmzn_logger_message_type, std::string> Logs;

int process_message_to_external(const char *message,void *command_window_void);

class MessageLog
{
public:

	MessageLog() : maximum_entries(500)
	{
	}

   void addEntry(cmzn_logger_message_type type, const char *message);
   int setMaximumNumberOfMessages(int number);
   int getNumberOfMessages();
   std::string getEntries(cmzn_logger_message_type type);
   void removeAllMessages();
	char *getMessageTextAtIndex(int index);
	enum cmzn_logger_message_type getMessageTypeAtIndex(int index);

	virtual ~MessageLog()
	{
	}

protected:
	int maximum_entries;
	std::deque<Logs> logs_deque;
};


struct cmzn_loggernotifier
{
private:
	cmzn_logger_id logger; // not accessed
	cmzn_loggernotifier_callback_function function;
	void *user_data;
	int access_count;

	cmzn_loggernotifier(cmzn_logger *logger);

	~cmzn_loggernotifier();

public:

	/** private: external code must use cmzn_logger_create_notifier */
	static cmzn_loggernotifier *create(cmzn_logger *logger)
	{
		if (logger)
			return new cmzn_loggernotifier(logger);
		return 0;
	}

	cmzn_loggernotifier *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_loggernotifier* &notifier);

	int setCallback(cmzn_loggernotifier_callback_function function_in,
		void *user_data_in);

	void *getUserData()
	{
		return this->user_data;
	}

	void clearCallback();

	void loggerDestroyed();

	void notify(cmzn_loggerevent *event)
	{
		if (this->function && event)
			(this->function)(event, this->user_data);
	}

};

struct cmzn_loggerevent
{
private:
	cmzn_logger *logger;
	cmzn_logger_change_flags changeFlags;
	cmzn_logger_message_type messageType;
	char *message;

	int access_count;

	cmzn_loggerevent(cmzn_logger *loggerIn);

	~cmzn_loggerevent();

public:

	/** @param loggerIn  Owning logger; can be NULL for FINAL event */
	static cmzn_loggerevent *create(cmzn_logger *loggerIn)
	{
		return new cmzn_loggerevent(loggerIn);
	}

	cmzn_loggerevent *access()
	{
		++(this->access_count);
		return this;
	}

	static int deaccess(cmzn_loggerevent* &event);

	char *getMessageText()
	{
		if (message)
			return duplicate_string(message);
		return 0;
	}

	char *setMessage(const char *messageIn)
	{
		if (message)
			DEALLOCATE(message);
		if (messageIn)
			message = duplicate_string(messageIn);
		return 0;
	}

	cmzn_logger_message_type getMessageType() const
	{
		return this->messageType;
	}

	void setMessageType(cmzn_logger_message_type messageTypeIn)
	{
		this->messageType = messageTypeIn;
	}

	cmzn_logger_change_flags getChangeFlags() const
	{
		return this->changeFlags;
	}

	void setChangeFlags(cmzn_logger_change_flags changeFlagsIn)
	{
		this->changeFlags = changeFlagsIn;
	}

	cmzn_logger *getLogger()
	{
		return cmzn_logger_access(this->logger);
	}
};

typedef std::list<cmzn_loggernotifier *> cmzn_loggernotifier_list;

struct cmzn_logger : public MessageLog
{
private:

	int access_count;
	bool enable;

	cmzn_logger();

public:

	cmzn_loggernotifier_list *notifier_list;

	~cmzn_logger()
	{
		for (cmzn_loggernotifier_list::iterator iter = notifier_list->begin();
			iter != notifier_list->end(); ++iter)
		{
			cmzn_loggernotifier *notifier = *iter;
			notifier->loggerDestroyed();
			cmzn_loggernotifier::deaccess(notifier);
		}
		delete notifier_list;
		notifier_list = 0;

		set_display_message_function(0, 0);
	}

	static cmzn_logger *create()
	{
		return new cmzn_logger();
	}

	cmzn_logger *access()
	{
		++(this->access_count);
		return this;
	}

	void addNotifier(cmzn_loggernotifier *notifier);

	void removeNotifier(cmzn_loggernotifier *notifier);

	static int deaccess(cmzn_logger* &logger)
	{
		if (logger)
		{
			--(logger->access_count);
			if (logger->access_count <= 0)
				delete logger;
			logger = 0;
			return CMZN_OK;
		}
		return CMZN_ERROR_ARGUMENT;
	}

};

#endif
