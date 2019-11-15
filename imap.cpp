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
    //TBC: CHECK if status information should be obtained BEFORE the data status above is freed.
    // //NOTE that mailimap set new interval ONLY gets a set with a single interval.ALSO need to confirm if item is to be freed.
    struct mailimap_set* mailimapSetPtr = mailimap_set_new_interval(1,0);
    // //function creates a fetch type structure
    struct mailimap_fetch_type* fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
    //creates a imap fetch att structure to request unique ID to msg
    struct mailimap_fetch_att* fetch_att = mailimap_fetch_att_new_uid();
    //adds a given fetch attribute to the mailimap fetch structure, in this case list of UIDs
    mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);
    //basically just initialise a clist bc the clist is JUST filled in.
    clist* fetch_result;
    //declare iterator to clist
    clistiter* cur;
    //retrieve data associated with given message numbers.In this case, UID numbers
    check_error(mailimap_fetch(session, mailimapSetPtr, fetch_type, &fetch_result), "MAILIMAP_ERROR_FETCH");

    msgCount = get_msg_count();
    // //initialising array to store messages
    // //http://www.fredosaurus.com/notes-cpp/newdelete/50dynamalloc.html
    msgPtr = new Message*[msgCount];

    int messageArrayPoint = 0;
    // Going through list of UID messages
    for(cur=clist_begin(fetch_result); cur !=NULL; cur = clist_next(cur))
    {
      struct mailimap_msg_att* msg_att;
      uint32_t uid;
    //get the list of attributes from a clist elmeent
      msg_att=(struct mailimap_msg_att*)clist_content(cur);
      uid=get_uid(msg_att);
    //skip over if UID is 0.
      if(uid==0)
      {
        continue;
      }
      msgPtr[messageArrayPoint]=new Message(session,uid, msgCount);
      messageArrayPoint++;
    //   *(msgPtr[i]) = Message(session,uid, msgCount);
    //   msgPtr[i]->fetch_msg(session,uid);messageArrayPoint
    }
    //MAKE SURE that you find a way to free the message pointer later on
    // Message**msgPtrPtr = &msgPtr;
    mailimap_fetch_list_free(fetch_result);
    return msgPtr;
}

//get uid for ALL Messages (Seems like it's only for one message??)
uint32_t Session:: get_uid(struct mailimap_msg_att* msg_att)
{
  //documentation on clist avail here: https://github.com/dinhviethoa/libetpan/blob/master/src/data-types/clist.h. Basically returns an iterator to the clist
  clistiter* cur;

  //iterate on each result of one given message
  for(cur=clist_begin(msg_att->att_list); cur!=NULL; cur=clist_next(cur))
  {
    //so basically you're iterating through the clist to find the UID.
    struct mailimap_msg_att_item* item;
    item=(mailimap_msg_att_item*)clist_content(cur);
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

int Session::get_msg_count()
{
  // function here gets status information
  // Create status attribute list pointer
  struct mailimap_status_att_list* attListPtr = mailimap_status_att_list_new_empty();
  // //adds status attributes to the list. how to get the number for status att?
  check_error(mailimap_status_att_list_add(attListPtr, MAILIMAP_STATUS_ATT_MESSAGES),"Status add for messages failed");
  struct mailimap_mailbox_data_status* dataStatus;
  check_error(mailimap_status(session, this->mb, attListPtr, &dataStatus), "Obtain ");
  //CHECK if this is correct but the goal here is to get inside to obtain msg number
  clist* theList = dataStatus -> st_info_list;
  auto value = ((mailimap_status_info*)(clist_content(clist_begin(theList))))->st_value;
  mailimap_mailbox_data_status_free(dataStatus);
  mailimap_status_att_list_free(attListPtr);
  return value;
}

char* Message::get_msg_att_msg_content(struct mailimap_msg_att * msg_att)
{
  /* iterate on each result of one given message */
  for(clistiter* cur = clist_begin(msg_att->att_list) ; cur != NULL ; cur = clist_next(cur)) {
    struct mailimap_msg_att_item * item;

    item = (mailimap_msg_att_item*)clist_content(cur);
    if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
      continue;
    }

    if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_BODY_SECTION) {
      continue;
    }

    return item->att_data.att_static->att_data.att_body_section->sec_body_part;
  }

  return NULL;
}

char* Message::get_msg_header(struct mailimap_msg_att * msg_att)
{
  /* iterate on each result of one given message */
  for(clistiter* cur = clist_begin(msg_att->att_list) ; cur != NULL ; cur = clist_next(cur)) {
    struct mailimap_msg_att_item * item;

    item = (mailimap_msg_att_item*)clist_content(cur);
    if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
      continue;
    }

    if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_RFC822_HEADER) {
      continue;
    }

    return item->att_data.att_static-> att_data.att_rfc822_header.att_content;
  }

  return NULL;
}

//
//
// // //quite obviously the case that this should be in the message class--how to link?
//  char* Message::get_msg_content(clist* fetch_result, size_t* p_msg_size)
// {
//   clistiter* cur;
//   /*iterate through each message*/
//   for(cur = clist_begin(fetch_result); cur!=NULL; cur=clist_next(cur))
//   {
//     struct mailimap_msg_att* msg_att;
//     //https://www.geeksforgeeks.org/size_t-data-type-c-language/
//     size_t msg_size;
//     char* msg_content;
//
//     msg_att=(mailimap_msg_att*)clist_content(cur);
//     msg_content = get_msg_att_msg_content(msg_att, &msg_size);
//     if(msg_content==NULL)
//     {
//       continue;
//     }
//
//     *p_msg_size=msg_size;
//     return msg_content;
//   }
//   return NULL;
// };

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
  //CHECK if this is the right place to free
  // free(msgPtr);
  //also need to free the  messages you just had
}

std::string Message::getBody()
{
  /*NEW SOLUTION*/
  //NOTE alternative function to fetch body peek section in Csample
  struct mailimap_section * section = mailimap_section_new(NULL);
  struct mailimap_fetch_att* fetchBodyAtt = mailimap_fetch_att_new_body_section(section);
  //create fetch type
  struct mailimap_fetch_type* fetchType = mailimap_fetch_type_new_fetch_att_list_empty();
  //put fetch attribute into fetch type
  mailimap_fetch_type_new_fetch_att_list_add(fetchType, fetchBodyAtt);
  //creates a set with a SINGLE message
  struct mailimap_set* mailimapSetSingle = mailimap_set_new_single(uid);
  //create a new clist storage structure
  clist* fetchResult;
    //fetch the body into clist** result
  mailimap_uid_fetch(session, mailimapSetSingle, fetchType, &fetchResult);
  // NEED TO CHECK IF THERE'S A BETTER WAY TO ACCESS THIS. WHAT'S the right declaration to get the ideal outcome?
  clistiter* cur;

  for(cur=clist_begin(fetchResult); cur!=NULL; cur=clist_next(cur))
  {
    struct mailimap_msg_att* msg_att = (mailimap_msg_att*)clist_content(cur);
    char* msg_content = get_msg_att_msg_content(msg_att);
    if(msg_content==NULL)
    {
      continue;
    }
    std::string msg = std::string(msg_content);
    mailimap_fetch_list_free(fetchResult);
    return msg;
  }
  //this gets you message attribute which you then need to break down
  //att_list details access
  std::string empty = "empty";
  mailimap_fetch_list_free(fetchResult);
  return empty;
}

//STOPPED HERE. REVIEW HOW MESSAGE IS RETURNED.
std::string Message::getField(std::string fieldname)
{
  //OLD solution
  //so it seems like you need to use the beginning section attributes to get the headers?
  // clist* clist_hdr_list;
  // struct mailimap_header_list* hdrList =  mailimap_header_list_new(clist_hdr_list);
  // struct mailimap_section* mailimapHeaderFields = mailimap_section_new_header_fields(hdrList);
  // struct mailimap_fetch_att* fetchAttHeader =  mailimap_fetch_att_new_body_section(mailimapHeaderFields);
  // struct mailimap_fetch_type* fetchType =  mailimap_fetch_type_new_fetch_att_list_empty();
  // mailimap_fetch_type_new_fetch_att_list_add(fetchType, fetchAttHeader);
  // struct mailimap_set* mailimapSetSingle = mailimap_set_new_single(uid);
  // //create a storage structure for fetch result
  // clist* fetchResult;
  // struct mailimap_status_att_list_ add

  // CURRENT SOLUTION
  // struct mailimap_fetch_att* fetchHeaderAtt = mailimap_fetch_att_new_rfc822_header();
  // //create fetch type
  // struct mailimap_fetch_type* fetchType = mailimap_fetch_type_new_fetch_att_list_empty();
  // //put fetch attribute into fetch type
  // mailimap_fetch_type_new_fetch_att_list_add(fetchType, fetchHeaderAtt);
  // //creates a set with a SINGLE message
  // struct mailimap_set* mailimapSetSingle = mailimap_set_new_single(uid);
  // //create a new clist storage structure
  // clist* fetchResult;
  //   //fetch the body into clist** result
  // mailimap_uid_fetch(session, mailimapSetSingle, fetchType, &fetchResult);
  // // NEED TO CHECK IF THERE'S A BETTER WAY TO ACCESS THIS. WHAT'S the right declaration to get the ideal outcome?
  // clistiter* cur;
  //
  // for(clistiter* cur=clist_begin(fetchResult); cur!=NULL; cur=clist_next(cur))
  // {
  //   struct mailimap_msg_att* msg_att = (mailimap_msg_att*)clist_content(cur);
  //   // char* msg_content = get_msg_header(msg_att);
  //   // if(msg_content==NULL)
  //   // {
  //   //   continue;
  //   // }
  //   // std::string header = std::string(msg_content);
  //   mailimap_fetch_list_free(fetchResult);
  //   std::string empty = "empty";
  //   return empty;
  // }
  // //remember to FREE fetch type later
  //check if SECTION needs to be freed as well
  std::string empty = "empty";
  // mailimap_fetch_list_free(fetchResult);
  return empty;
}

void Message::deleteFromMailbox()
{
}
