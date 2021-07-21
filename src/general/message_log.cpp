/**
 * @file message_log.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "general/message_log.hpp"
#include "opencmiss/zinc/types/loggerid.h"
#include <algorithm>
#include <sstream>

std::string Message_type_to_string(Message_type type)
{
	switch (type)
	{
		case ERROR_MESSAGE:
		{
			return std::string("ERROR_MESSAGE");
		} break;
		case WARNING_MESSAGE:
		{
			return std::string("WARNING_MESSAGE");
		} break;
		case INFORMATION_MESSAGE:
		{
			return std::string("INFORMATION_MESSAGE");
		} break;
	}
	return 0;
}

enum cmzn_logger_message_type convertMessageTypeToExternalType(enum Message_type type)
{
	switch (type)
	{
		case ERROR_MESSAGE:
		{
			return CMZN_LOGGER_MESSAGE_TYPE_ERROR;
		} break;
		case WARNING_MESSAGE:
		{
			return CMZN_LOGGER_MESSAGE_TYPE_WARNING;
		} break;
		case INFORMATION_MESSAGE:
		{
			return CMZN_LOGGER_MESSAGE_TYPE_INFORMATION;
		} break;
		default:
		{
			return CMZN_LOGGER_MESSAGE_TYPE_INVALID;
		} break;
	}
}

void MessageLog::addEntry(cmzn_logger_message_type type, const char *message)
{
	std::string messageString(message);
	if (logs_deque.size() >= (unsigned int)maximum_entries)
		logs_deque.pop_front();
	logs_deque.push_back(std::make_pair(type, messageString));
}

int MessageLog::getNumberOfMessages()
{
	return static_cast<int>(logs_deque.size());
}

enum cmzn_logger_message_type MessageLog::getMessageTypeAtIndex(int index)
{
	if (index < 1 || logs_deque.size() < (unsigned int)index)
	{
		return CMZN_LOGGER_MESSAGE_TYPE_INVALID;
	}
	else
	{
		return logs_deque[index-1].first;
	}
	return CMZN_LOGGER_MESSAGE_TYPE_INVALID;
}

char *MessageLog::getMessageTextAtIndex(int index)
{
	if (index < 1 || logs_deque.size() < (unsigned int)index)
	{
		return 0;
	}
	else
	{
		return duplicate_string(logs_deque[index-1].second.c_str());
	}
	return 0;
}

void MessageLog::removeAllMessages()
{
	logs_deque.clear();
}

int MessageLog::setMaximumNumberOfMessages(int number)
{
	if (number > 0)
	{
		maximum_entries = number;
		if (logs_deque.size() > (unsigned int)maximum_entries)
		{
			const size_t differences = logs_deque.size() - maximum_entries;
			logs_deque.erase(logs_deque.begin(), logs_deque.begin() + differences);
		}
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

std::string MessageLog::getEntries(cmzn_logger_message_type type)
{
	std::ostringstream ss;
	for (std::deque<Logs>::iterator iter = logs_deque.begin();
		iter != logs_deque.end(); iter++)
	{
		if ((*iter).first == type)
		{
			ss << (*iter).second;
		}
	}

	return ss.str();
}

static int log_message(const char *message, Message_type type, void *logger_void)
{
	if (logger_void)
	{
		cmzn_logger *logger = (cmzn_logger *)logger_void;
		if (logger)
		{
			logger->addEntry(convertMessageTypeToExternalType(type), message);
			if (0 < logger->notifier_list->size())
			{
				cmzn_loggerevent_id event = cmzn_loggerevent::create(logger);
				event->setMessage(message);
				event->setMessageType(convertMessageTypeToExternalType(type));
				event->setChangeFlags(CMZN_LOGGER_CHANGE_FLAG_NEW_MESSAGE);
				for (cmzn_loggernotifier_list::iterator iter = logger->notifier_list->begin();
					iter != logger->notifier_list->end(); ++iter)
				{
					(*iter)->notify(event);
				}
				cmzn_loggerevent::deaccess(event);
			}
			return 1;
		}
	}

	return 0;
}

cmzn_loggernotifier::cmzn_loggernotifier(
	cmzn_logger *logger) :
	logger(logger),
	function(0),
	user_data(0),
	access_count(1)
{
	logger->addNotifier(this);
}

cmzn_loggernotifier::~cmzn_loggernotifier()
{
}

int cmzn_loggernotifier::deaccess(cmzn_loggernotifier* &notifier)
{
	if (notifier)
	{
		--(notifier->access_count);
		if (notifier->access_count <= 0)
			delete notifier;
		else if ((1 == notifier->access_count) && notifier->logger)
			notifier->logger->removeNotifier(notifier);
		notifier = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_loggernotifier::setCallback(cmzn_loggernotifier_callback_function function_in,
	void *user_data_in)
{
	if (!function_in)
		return CMZN_ERROR_ARGUMENT;
	this->function = function_in;
	this->user_data = user_data_in;
	return CMZN_OK;
}

void cmzn_loggernotifier::clearCallback()
{
	this->function = 0;
	this->user_data = 0;
}

void cmzn_loggernotifier::loggerDestroyed()
{
	this->logger = 0;
	if (this->function)
	{
		cmzn_loggerevent_id event = cmzn_loggerevent::create(
			static_cast<cmzn_logger*>(0));
		event->setChangeFlags(CMZN_LOGGER_CHANGE_FLAG_FINAL);
		(this->function)(event, this->user_data);
		cmzn_loggerevent::deaccess(event);
		this->clearCallback();
	}
}

cmzn_loggerevent::cmzn_loggerevent(
	cmzn_logger *loggerIn) :
	logger(cmzn_logger_access(loggerIn)),
	changeFlags(CMZN_LOGGER_CHANGE_FLAG_NONE),
	messageType(CMZN_LOGGER_MESSAGE_TYPE_INVALID),
	message(0),
	access_count(1)
{
}

cmzn_loggerevent::~cmzn_loggerevent()
{
	if (message)
		DEALLOCATE(message);
	cmzn_logger_destroy(&this->logger);
}

int cmzn_loggerevent::deaccess(cmzn_loggerevent* &event)
{
	if (event)
	{
		--(event->access_count);
		if (event->access_count <= 0)
			delete event;
		event = 0;
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_logger::cmzn_logger() : MessageLog(), access_count(1)
{
	notifier_list = new cmzn_loggernotifier_list();
	enable = 0;
	set_display_message_function(log_message, (void *)this);
}


void cmzn_logger::addNotifier(cmzn_loggernotifier *notifier)
{
	notifier_list->push_back(notifier->access());
}

void cmzn_logger::removeNotifier(cmzn_loggernotifier *notifier)
{
	if (notifier)
	{
		cmzn_loggernotifier_list::iterator iter = std::find(
			notifier_list->begin(), notifier_list->end(), notifier);
		if (iter != notifier_list->end())
		{
			cmzn_loggernotifier::deaccess(notifier);
			notifier_list->erase(iter);
		}
	}
}

int cmzn_logger_remove_all_messages(cmzn_logger_id logger)
{
	if (logger)
	{
		logger->removeAllMessages();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_logger_get_number_of_messages(cmzn_logger_id logger)
{
	if (logger)
	{
		return logger->getNumberOfMessages();
	}
	return 0;
}

enum cmzn_logger_message_type cmzn_logger_get_message_type_at_index(
	cmzn_logger_id logger, int index)
{
	if (logger)
	{
		return logger->getMessageTypeAtIndex(index);
	}
	return CMZN_LOGGER_MESSAGE_TYPE_INVALID;
}

char *cmzn_logger_get_message_text_at_index(cmzn_logger_id logger, int index)
{
	if (logger)
	{
		return logger->getMessageTextAtIndex(index);
	}
	return 0;
}

int cmzn_logger_set_maximum_number_of_messages(cmzn_logger_id logger, int number)
{
	if (logger)
	{
		return logger->setMaximumNumberOfMessages(number);
	}
	return CMZN_ERROR_ARGUMENT;
}

cmzn_logger_id cmzn_logger_access(cmzn_logger_id logger)
{
	if (logger)
		return logger->access();
	return 0;
}

int cmzn_logger_destroy(cmzn_logger_id *logger_address)
{
	if (logger_address)
		return cmzn_logger::deaccess(*logger_address);
	return CMZN_ERROR_ARGUMENT;
}

cmzn_loggernotifier_id cmzn_logger_create_loggernotifier(
	cmzn_logger_id logger)
{
	return cmzn_loggernotifier::create(logger);
}

int cmzn_loggernotifier_clear_callback(
	cmzn_loggernotifier_id notifier)
{
	if (notifier)
	{
		notifier->clearCallback();
		return CMZN_OK;
	}
	return CMZN_ERROR_ARGUMENT;
}

int cmzn_loggernotifier_set_callback(cmzn_loggernotifier_id notifier,
	cmzn_loggernotifier_callback_function function_in, void *user_data_in)
{
	if (notifier && function_in)
		return notifier->setCallback(function_in, user_data_in);
	return CMZN_ERROR_ARGUMENT;
}

void *cmzn_loggernotifier_get_callback_user_data(
 cmzn_loggernotifier_id notifier)
{
	if (notifier)
		return notifier->getUserData();
	return 0;
}

cmzn_loggernotifier_id cmzn_loggernotifier_access(
	cmzn_loggernotifier_id notifier)
{
	if (notifier)
		return notifier->access();
	return 0;
}

int cmzn_loggernotifier_destroy(cmzn_loggernotifier_id *notifier_address)
{
	return cmzn_loggernotifier::deaccess(*notifier_address);
}

cmzn_loggerevent_id cmzn_loggerevent_access(cmzn_loggerevent_id event)
{
	if (event)
		return event->access();
	return 0;
}

int cmzn_loggerevent_destroy(cmzn_loggerevent_id *event_address)
{
	return cmzn_loggerevent::deaccess(*event_address);
}

cmzn_logger_change_flags cmzn_loggerevent_get_change_flags(
	cmzn_loggerevent_id event)
{
	if (event)
		return event->getChangeFlags();
	return CMZN_LOGGER_CHANGE_FLAG_NONE;
}

cmzn_logger_message_type cmzn_loggerevent_get_message_type(
	cmzn_loggerevent_id event)
{
	if (event)
		return event->getMessageType();
	return CMZN_LOGGER_MESSAGE_TYPE_INVALID;
}


char *cmzn_loggerevent_get_message_text(cmzn_loggerevent_id event)
{
	if (event)
		return event->getMessageText();
	return 0;
}

cmzn_logger_id cmzn_loggerevent_get_logger(cmzn_loggerevent_id event)
{
	if (event)
		return event->getLogger();
	return 0;
}
