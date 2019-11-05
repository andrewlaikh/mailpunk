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
    struct mailimap_set* mailimapSetPtr = mailimap_set_new_interval(1,0);
    //function creates a fetch type structure
    struct mailimap_fetch_type* fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
    //creates a imap fetch att structure to request unique ID to msg
    struct mailimap_fetch_att* fetch_att = mailimap_fetch_att_new_uid();
    //adds a given fetch attribute to the mailimap fetch structure
    mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);

    //basically just initialise a clist bc the clist is JUST filled in.
    clist* fetch_result;
    //retrieve data associated with given message numbers.
    check_error(mailimap_fetch(session, mailimapSetPtr, fetch_type, &fetch_result), "MAILIMAP_ERROR_FETCH");

    for(curr=clist_begin(fetch_results); curr !=NULL; cur = clist_next(cur))
    {
      struct mailimap_msg_att* msg_att;
      uint32_t uid;

      msg_att=clist_content(curr);
      uid=get_uid(msg_att);
      if(uid==0)
      {
        continue;
      }
      //thought process is need to find someway to declare a new message class to get details?
      fetch_msg(session,uid);
    }

    Message* a = new Message;
    Message** b = &a;
    return b;
}

//get uid for ALL Messages
static uint32_t Session:: get_uid(struct mailimap_msg_att* msg_att)
{
  //documentation on clist avail here: https://github.com/dinhviethoa/libetpan/blob/master/src/data-types/clist.h. Basically returns an iterator to the clist
  clistiter* cur;

  //iterate on each result of one given message
  for(cur=clist_begin(msg_att->att_list); cur!=NULL; cur=clist_next(cur))
  {
    //so basically you're iterating through the clist to find the UID.
    struct mailimap_msg_att_item* item;
    item=clist_content(cur);
    if(item->att_type!=MAILIMAP_MSG_ATT_ITEM_STATIC)
    {
      continue;
    }
    //again, what does the below variable mean?
    if(item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_UID)
    {
      continue;
    }
    return item->att_data.att_static->att_data.att_uid;
  }
  //IF nothing in the clist is found with the matching details
  return 0;
}

//helper function to get messages. BUT should you have a friend function instead OR something else so that information can be obtained?
void Session::fetch_msg(struct mailimap* imap, uint32_t uid)
{
  //cheat sheet has a function here to check whether mail already fetched. Skipped over.

  set = mailimap_set_new_single(uid);
  //creates a new mailimap fetch type structure
  struct mailimap_fetch_type* fetch_type=mailimap_fetch_type_new_att_list_empty();
  //creates a new mailimap section structure. Note that the function defined merely introduces a mailimap section structure to describe a list of headers to be fetched.
  struct mailimap_section* section=mailimap_section_new(NULL);
  //so the suggestion in the document is to get peek section.
  struct mailimap_fetch_att* fetch_att=mailimap_fetch_att_new_body_peek_section(section);
  //adds a given fetch attribute structure to the mailimap fetch structure
  mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);
  //declare a clist for fetch_result (i.e. where all the clist are stored)
  clist* fetch_result;
  //retrieve data associated with the given message numbers
  check_error(mailimap_uid_fetch(session, set, fetch_type, &fetch_result), "MAILIMAP_ERROR_UID_FETCH");
  //cheat sheet has a print error message function, skipped over.

  size_t msg_len;
  char* msg_content=get_msg_content(fetch_result, &msg_len);
  if(msg_content==NULL)
  {
    fprintf(stfderr, "no content\n");
    mailimap_fetch_list_free(fetch_result);
    return;
  }

//Object type that identifies a stream and contains the information needed to control it
//http://www.cplusplus.com/reference/cstdio/fopen/ Basically open file with a stream that can be identified.
  FILE* f=fopen(filename,"w");

  //reminder f is a pointer
  if(f==NULL)
  {
    fprintf(stderr, "could not write\n");
    mailimap_fetch_list_free(fetch_result);
    return;
  }

  //http://www.cplusplus.com/reference/cstdio/fwrite/
  //fwrite basically writes an array to the stream chosen
  //first argument is pointer to stream to be copied
  //second argument is byte size of each object to be copied
  //third argument is the count as to the number of messages
  //fourth argument is the current position in the stream to be written to.
  fwrite(msg_content, 1, msg_len, f);
  //my thoughts: just declare a message class and write into message class.

  //basically closes the stream.
  fclose(f);


  //model has function to show that message has been fetched.
  mailimap_fetch_list_free(fetch_result);
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
