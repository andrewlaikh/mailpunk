#include "imap.hpp"
//guide to clists: https://docs.microsoft.com/en-us/cpp/mfc/reference/clist-class?view=vs-2019

using namespace IMAP;

//session is used with the 'function' container in line 48 of UI.CPP
Session::Session(std::function<void()> updateUI)
{
  //set to 0 and null according to the defaults. Creates a new IMAP session. It's returned as a pointer.
  session = mailimap_new(0, NULL);
}

Message** Session::getMessages()
{
    //function here gets status information
    // Create status attribute list pointer
	  struct mailimap_status_att_list* attListPtr = mailimap_status_att_list_new_empty();
    //adds status attributes to the list. how to get the number for status att?
    check_error(mailimap_status_att_list_add(attListPtr, MAILIMAP_STATUS_ATT_MESSAGES),"Status add for messages failed");
    struct mailimap_mailbox_data_status dataStatus;
    struct mailimap_mailbox_data_status* resultPtr = &dataStatus;
    check_error(mailimap_status(session, this->mb, attListPtr, &resultPtr), "Obtain ");
    mailimap_mailbox_data_status_free(resultPtr);
    mailimap_status_att_list_free(attListPtr);
    //end of search for status information

    //TBC: CHECK if status information should be obtained BEFORE the data status above is freed.
    //NOTE that mailimap set new interval ONLY gets a set with a single interval.ALSO need to confirm if item is to be freed.
    // struct mailimap_set* mailimapSetPtr = mailimap_set_new_interval(0,1);
    //
    // mailimap_fetch();

    Message* a = new Message;
    Message** b = &a;
    return b;
}

//const or not?
void Session::connect(std::string const& server, size_t port)
{
  //Based on line 48 of UI.CPP, we should already have a session in place from constructor function.
  //https://stackoverflow.com/questions/45268493/cannot-convert-string-to-char-for-argument-error
  //note on pointer to const string: http://www.cplusplus.com/reference/string/string/c_str/
  mailimap_socket_connect(this->session, server.c_str(), port);
}

//const or not?
void Session::login(std::string const& userid, std::string const& password)
{
  check_error(mailimap_login(this->session, userid.c_str(), password.c_str()), "could not login");
}

//const or not?
void Session::selectMailbox(std::string const& mailbox)
{
  //pointer to mb for future use
  int length = mailbox.length();
  char mb_array[length+1];
  strcpy(mb_array, mailbox.c_str());
  this-> mb = mb_array;
  //simply converting the string constants using pointers.
  check_error(mailimap_select(this->session, mailbox.c_str()), "could not select mailbox");
}

// left as is for now, review what to put in destructor
Session::~Session()
{
  mailimap_free(session);
}

std::string Message:: getBody()
{
  std::string a = "DummyMsg";
  return a;
}

std::string Message::getField(std::string fieldname)
{
  std::string a = "DummyMsg";
  return a;
}

void Message::deleteFromMailbox()
{
}
