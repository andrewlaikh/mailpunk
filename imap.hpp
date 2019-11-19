#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <string>
#include <functional>
#include <cstring>


namespace IMAP {
	class Session;

	class Message {
	public:
		Message(Session* session, uint32_t uid, std::function<void()> updateUI):sessionPtr(session), uid(uid), updateUI(updateUI){};
		//
		std::string getBody();
		/**
		 * Get one of the descriptor fields (subject, from, ...)
		 */
		std::string getField(std::string fieldname);
		/**
		 * Remove this mail from its mailbox
		 */
		// self-defined function to get msg att, worth modifying into string to return
		std::string get_msg_content(struct mailimap_msg_att * msg_att);
		//  self-defined function to get msg header
		std::string get_msg_header(struct mailimap_msg_att * msg_att, std::string fieldname);

		//free content/header
		void freeHelperFunction(struct mailimap_set* mailimapSetSingle, clist* fetchResult, struct mailimap_fetch_type* fetchType);

		void deleteFromMailbox();

		//return empty mailbox to prevent mem leaks.
		void setMsgEmpty();

		uint32_t uid;

	private:
		Session* sessionPtr;
		std::function<void()> updateUI;

	};

class Session {
public:
	//constructor for session
	//http://www.cplusplus.com/reference/functional/function/?kw=function
	//https://stackoverflow.com/questions/20353210/usage-and-syntax-of-stdfunction
	Session(std::function<void()> updateUI);
	/**
	* Get all messages in the INBOX mailbox terminated by a nullptr (like we did in class)
	*/
	Message** getMessages();
	/**
	* connect to the specified server (143 is the standard unencrypte imap port)
	*/
	//self-defined function
	uint32_t get_uid(struct mailimap_msg_att* msg_att);

	//self-defined function
	int get_msg_count();

	void connect(std::string const& server, size_t port = 143);

	/**
	* log in to the server (connect first, then log in)
	*/
	void login(std::string const& userid, std::string const& password);
	/**
	* select a mailbox (only one can be selected at any given time)
	*
	// * this can only be performed after login
	*/
	void selectMailbox(std::string const& mailbox);

	//be careful with the below. SElf-defined function
	void deleteAllMail();

	~Session();

	//also consider whether destruction is needed, but not imapSession in destructor
	Message** msgPtr = NULL;
	std::function<void()> updateUI;
	int msgCount;
	mailimap* imapSession = NULL;

private:
	std::string mailboxStore;
};
}

#endif /* IMAP_H */
