#include "pch.h"
#include "sequence.h"

bool sequence::init(/*const seqchar* buffer, size_t length, bool duplicate_buf= false*/ size_w length, const read_fn& read)
{
//	buffer_ = buffer;
//	length_ = length;

	length_ = length;

	read_ = read;

	//if(!clear())
	//	return false;

	//buffer_control *bc = new buffer_control();
	//bc->init(buffer, length, duplicate_buf);

	//bc->id = (int)buffer_list.size();		// assign the id
	//buffer_list.push_back(bc);

	///*buffer_control *bc = alloc_modifybuffer(length);
	//memcpy(bc->buffer, buffer, length * sizeof(seqchar));
	//bc->length = length;*/

	//span *sptr = new span(0, length, bc->id, tail, head);
	//head->next = sptr;
	//tail->prev = sptr;

	//sequence_length = length;
	return true;
}


size_t sequence::render(size_w index, seqchar* dest, size_t length, seqchar_info* infobuf/*= nullptr*/) const
{
	size_t len= 0;

	if (read_)
		len = read_(index, dest, length);
	else
		return 0;
/*
	if (buffer_ == nullptr)
		return 0;

	const seqchar* end= buffer_ + length_;
	const seqchar* p= buffer_ + index;

	size_t len= min(length, static_cast<size_t>(p < end ? end - p : 0));

	memcpy(dest, p, len * sizeof(seqchar));
*/
	if (infobuf)
	{
		for (size_t i = 0; i < len; ++i)
		{
			infobuf[i].buffer	= 0;//sptr->buffer;
			infobuf[i].userdata = 0;
		}

		infobuf += len;
	}

	return len;
}


bool sequence::replace(size_w index, const seqchar* buf, size_w length)
{
	return true;
}


bool sequence::erase(size_w index)
{ return true; }


bool sequence::erase(size_w index, size_w len)
{ return true; }
