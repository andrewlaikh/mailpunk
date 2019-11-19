#include "imap.hpp"

using namespace IMAP;

Session::Session(std::function<void()> updateUI):updateUI(updateUI)
{
  imapSession = mailimap_new(0, NULL);
}

Message** Session::getMessages()
{
    msgCount = get_msg_count();
    int msgArrayLength = msgCount+1;

    //stop getMessages if message count is 0.
    if(msgCount == 0)
    {
      msgPtr = new Message*[0];
      //set NULL so that there is no seg fault when accessing
      msgPtr[0] = NULL;
      return msgPtr;
    }

    struct mailimap_set* mailimapSetPtr = mailimap_set_new_interval(1,0);
    struct mailimap_fetch_type* fetch_type = mailimap_fetch_type_new_fetch_att_list_empty();
    struct mailimap_fetch_att* fetch_att = mailimap_fetch_att_new_uid();
    mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);
    clist* fetch_result;
    clistiter* cur;
    check_error(mailimap_fetch(imapSession, mailimapSetPtr, fetch_type, &fetch_result), "MAILIMAP_ERROR_FETCH");

    // //initialising array to store messages
    // //http://www.fredosaurus.com/notes-cpp/newdelete/50dynamalloc.html
    msgPtr = new Message*[msgArrayLength];
    Session* sessionPtr = this;

    int messageArrayIterator = 0;
    // Going through list of UID messages
    for(cur=clist_begin(fetch_result); cur !=NULL; cur = clist_next(cur))
    {
      struct mailimap_msg_att* msg_att;
      uint32_t uid;
      msg_att=(struct mailimap_msg_att*)clist_content(cur);
      uid=get_uid(msg_att);
      //skip over if UID is invalid. Be Careful.
      if(uid>0)
      {
        msgPtr[messageArrayIterator]=new Message(sessionPtr, uid, updateUI);
        messageArrayIterator++;
      }
    }
    msgPtr[messageArrayIterator]=NULL;
    mailimap_fetch_list_free(fetch_result);
    //be careful with this
    mailimap_fetch_type_free(fetch_type);
    mailimap_set_free(mailimapSetPtr);
    return msgPtr;
}

uint32_t Session:: get_uid(struct mailimap_msg_att* msg_att)
{
  clistiter* cur;

  for(cur=clist_begin(msg_att->att_list); cur!=NULL; cur=clist_next(cur))
  {
    struct mailimap_msg_att_item* item;
    item=(mailimap_msg_att_item*)clist_content(cur);
    if(item->att_type!=MAILIMAP_MSG_ATT_ITEM_STATIC)
    {
      continue;
    }
    if(item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_UID)
    {
      continue;
    }
    return item->att_data.att_static->att_data.att_uid;
  }
  return 0;
}

int Session::get_msg_count()
{
  struct mailimap_status_att_list* attListPtr = mailimap_status_att_list_new_empty();
  check_error(mailimap_status_att_list_add(attListPtr, MAILIMAP_STATUS_ATT_MESSAGES),"Status add for messages failed");
  struct mailimap_mailbox_data_status* dataStatus;
  const char* mb = mailboxStore.c_str();
  check_error(mailimap_status(imapSession, mb, attListPtr, &dataStatus), "Obtain ");
  clist* theList = dataStatus -> st_info_list;
  auto value = ((mailimap_status_info*)(clist_content(clist_begin(theList))))->st_value;
  mailimap_mailbox_data_status_free(dataStatus);
  mailimap_status_att_list_free(attListPtr);
  return value;
}

std::string Message::get_msg_content(struct mailimap_msg_att * msg_att)
{
  for(clistiter* cur = clist_begin(msg_att->att_list) ; cur != NULL ; cur = clist_next(cur)) {
    struct mailimap_msg_att_item * item;

    item = (mailimap_msg_att_item*)clist_content(cur);
    if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
      continue;
    }

    if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_BODY_SECTION) {
      continue;
    }
      std::string msg = std::string(item->att_data.att_static->att_data.att_body_section->sec_body_part);
      return msg;
  }
  std::string empty = "empty";
  return empty;
}

std::string Message::get_msg_header(struct mailimap_msg_att* msg_att, std::string fieldname)
{
  for(clistiter* cur = clist_begin(msg_att->att_list) ; cur!= NULL ; cur = clist_next(cur)) {
    struct mailimap_msg_att_item * item;

    item = (mailimap_msg_att_item*)clist_content(cur);
    if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
      continue;
    }

    if (item->att_data.att_static->att_type!= MAILIMAP_MSG_ATT_ENVELOPE) {
      continue;
    }
    std::string Subject = "Subject";
    if (Subject.compare(fieldname) == 0)
    {
      std::string msg = std::string(item->att_data.att_static->att_data.att_env->env_subject);
      return msg;
    }
    std::string From = "From";
    if (From.compare(fieldname)==0)
    {
      clistiter* cur = clist_begin(item->att_data.att_static->att_data.att_env->env_from->frm_list);
      struct mailimap_address* msgAddress = (mailimap_address*)clist_content(cur);
      std::string name = std::string(msgAddress-> ad_mailbox_name);
      std::string mailbox = std::string(msgAddress->ad_host_name);
      return name + "@" + mailbox;
    }
  }
}

void Session::connect(std::string const& server, size_t port)
{
  //Based on line 48 of UI.CPP, we should already have a session in place from constructor function.
  //https://stackoverflow.com/questions/45268493/cannot-convert-string-to-char-for-argument-error
  //note on pointer to const string: http://www.cplusplus.com/reference/string/string/c_str/
  mailimap_socket_connect(this->imapSession, server.c_str(), port);
}

void Session::login(std::string const& userid, std::string const& password)
{
  check_error(mailimap_login(this->imapSession, userid.c_str(), password.c_str()), "could not login");
}

void Session::selectMailbox(std::string const& mailbox)
{
  mailboxStore.assign(mailbox);
  const char* mb = mailboxStore.c_str();
  check_error(mailimap_select(imapSession, mb), "could not select mailbox");
}

Session::~Session()
{
  for(int msgIterator = 0; msgIterator < msgCount; msgIterator++)
  {
    delete msgPtr[msgIterator];
  }
  delete [] msgPtr;
  mailimap_logout(imapSession);
  mailimap_free(imapSession);
}

std::string Message::getBody()
{
  struct mailimap_section* section = mailimap_section_new(NULL);
  struct mailimap_fetch_att* fetchAttBody = mailimap_fetch_att_new_body_section(section);
  struct mailimap_fetch_type* fetchType = mailimap_fetch_type_new_fetch_att_list_empty();
  mailimap_fetch_type_new_fetch_att_list_add(fetchType, fetchAttBody);
  struct mailimap_set* mailimapSetSingle = mailimap_set_new_single(uid);
  clist* fetchResult = NULL;
  mailimap_uid_fetch(sessionPtr->imapSession, mailimapSetSingle, fetchType, &fetchResult);

  for(clistiter* cur=clist_begin(fetchResult); cur!=NULL; cur=clist_next(cur))
  {
    struct mailimap_msg_att* msg_att = (mailimap_msg_att*)clist_content(cur);
    std::string msg_content = get_msg_content(msg_att);
    if(msg_content.empty())
    {
      continue;
    }
    freeHelperFunction(mailimapSetSingle, fetchResult, fetchType);
    return msg_content;
  }
  freeHelperFunction(mailimapSetSingle, fetchResult, fetchType);
  std::string empty = "empty";
  return empty;
}

std::string Message::getField(std::string fieldname)
{
  struct mailimap_fetch_att* fetchHeaderAtt=mailimap_fetch_att_new_envelope();
  struct mailimap_fetch_type* fetchType = mailimap_fetch_type_new_fetch_att_list_empty();
  mailimap_fetch_type_new_fetch_att_list_add(fetchType, fetchHeaderAtt);
  struct mailimap_set* mailimapSetSingle = mailimap_set_new_single(uid);
  clist* fetchResultHeader;
  mailimap_uid_fetch(sessionPtr->imapSession, mailimapSetSingle, fetchType, &fetchResultHeader);
  for(clistiter* cur=clist_begin(fetchResultHeader); cur!=NULL; cur=clist_next(cur))
  {
    struct mailimap_msg_att* msg_att = (mailimap_msg_att*)clist_content(cur);
    std::string msg_content = get_msg_header(msg_att, fieldname);
    if(msg_content.empty())
    {
      continue;
    }
    freeHelperFunction(mailimapSetSingle, fetchResultHeader, fetchType);
    return msg_content;
  }
  freeHelperFunction(mailimapSetSingle, fetchResultHeader, fetchType);
  std::string empty = "";
  return empty;
}

void Message::freeHelperFunction(struct mailimap_set* mailimapSetSingle, clist* fetchResult, struct mailimap_fetch_type* fetchType)
{
  mailimap_set_free(mailimapSetSingle);
  mailimap_fetch_list_free(fetchResult);
  mailimap_fetch_type_free(fetchType);

}

void Message::deleteFromMailbox()
{
  struct mailimap_flag_list* mailimapFlagList = mailimap_flag_list_new_empty();
  struct mailimap_flag* flagDelete = mailimap_flag_new_deleted();
  mailimap_flag_list_add(mailimapFlagList, flagDelete);
  struct mailimap_store_att_flags* storeAttFlags = mailimap_store_att_flags_new_set_flags(mailimapFlagList);
  struct mailimap_set* mailimapSetSingle = mailimap_set_new_single(uid);
  check_error(mailimap_uid_store(sessionPtr->imapSession, mailimapSetSingle, storeAttFlags), "Error storing flags");
  check_error(mailimap_expunge(sessionPtr->imapSession), "Error expunging flags");
  mailimap_store_att_flags_free(storeAttFlags);
  mailimap_set_free(mailimapSetSingle);
  sessionPtr->deleteAllMail();
  updateUI();
}

void Session::deleteAllMail()
{
  for(int i = 0; i < msgCount; i++)
  {
    delete msgPtr[i];
  }
  delete [] msgPtr;
  msgPtr = NULL;
}
