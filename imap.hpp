#ifndef IMAP_H
#define IMAP_H
#include "imaputils.hpp"
#include <libetpan/libetpan.h>
#include <string>
#include <functional>
#include <cstring>

namespace IMAP {
class Message {
public:
	//this is the constructor
	//use initializer list for session and uid
	Message(mailimap* session, uint32_t uid, int msgCount):session(session), uid(uid), msgCount(msgCount){}
	//
	//
	/**
	 * Get the body of the message. You may chose to either include the headers or not.
	 */
	std::string getBody();
	/**
	 * Get one of the descriptor fields (subject, from, ...)
	 */
	std::string getField(std::string fieldname);
	/**
	 * Remove this mail from its mailbox
	 */
	 //self-defined function to fetch message
	void fetch_msg(struct mailimap* imap, uint32_t uid);
	// //self-defined function to get
	// // char* get_msg_att_msg_content(struct mailimap_msg_att * msg_att, size_t* p_msg_size);
	// //
	// // char* get_msg_content(clist* fetch_result, size_t* p_msg_size);
	//
	void deleteFromMailbox();
	//not included, so is there a need for a destructor on the message side?

private:
	mailimap* session;
	uint32_t uid;
	//remember that this must be freed
	clist* fetch_result_storage;
	size_t* p_msg_size;
	//temp variable
	uint32_t msgCount;

};

class Session {
public:
	//constructor for session
	//http://www.cplusplus.com/reference/functional/function/?kw=function
	//function is a wrapper that can wrap and kind of callable element into a copyable object
	//https://stackoverflow.com/questions/20353210/usage-and-syntax-of-stdfunction
	Session(std::function<void()> updateUI);
	/**
	* Get all messages in the INBOX mailbox terminated by a nullptr (like we did in class)
	*/
	Message** getMessages();
	/**
	* connect to the specified server (143 is the standard unencrypte imap port)
	*/
	//self-defined function to fetch message content (should moved into message portion)
	//self-defined function to get message attribute message content

	//self-defined function to get UID for a specific message
	uint32_t get_uid(struct mailimap_msg_att* msg_att);
	//get message count
	int get_msg_count();

	void connect(std::string const& server, size_t port = 143);
	/**
	* log in to the server (connect first, then log in)
	*/
	void login(std::string const& userid, std::string const& password);
	/**
	* select a mailbox (only one can be selected at any given time)
	*
	* this can only be performed after login
	*/
	void selectMailbox(std::string const& mailbox);

	~Session();

	//also consider whether destruction is needed, but not imapSession in destructor

private:
	mailimap* session;
	Message** msgPtr = NULL;
	uint32_t UID = 36;
	//temp variable
	int msgCount;
	//NOT YET freed the underlying mailbox reference
	const char* mb;
};
}

#endif /* IMAP_H */
