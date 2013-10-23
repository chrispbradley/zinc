/**
 * FILE : selection.hpp
 */
/* OpenCMISS-Zinc Library
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef CMZN_SELECTION_HPP__
#define CMZN_SELECTION_HPP__

#include "zinc/selection.h"

namespace OpenCMISS
{
namespace Zinc
{

class Selectionevent
{
protected:
	cmzn_selectionevent_id id;

public:

	Selectionevent() : id(0)
	{  }

	// takes ownership of C handle, responsibility for destroying it
	explicit Selectionevent(cmzn_selectionevent_id in_selection_event_id) :
		id(in_selection_event_id)
	{  }

	Selectionevent(const Selectionevent& selectionEvent) :
		id(cmzn_selectionevent_access(selectionEvent.id))
	{  }

	enum ChangeType
	{
		CHANGE_NONE = CMZN_SELECTIONEVENT_CHANGE_NONE,
		CHANGE_ADD = CMZN_SELECTIONEVENT_CHANGE_ADD,
		CHANGE_REMOVE = CMZN_SELECTIONEVENT_CHANGE_REMOVE,
		CHANGE_FINAL = CMZN_SELECTIONEVENT_CHANGE_FINAL
	};

	Selectionevent& operator=(const Selectionevent& selectionEvent)
	{
		cmzn_selectionevent_id temp_id = cmzn_selectionevent_access(selectionEvent.id);
		if (0 != id)
		{
			cmzn_selectionevent_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Selectionevent()
	{
		if (0 != id)
		{
			cmzn_selectionevent_destroy(&id);
		}
	}

	bool isValid()
	{
		return (0 != id);
	}

	cmzn_selectionevent_id getId()
	{
		return id;
	}

	int getChangeSummary() const
	{
		return static_cast<ChangeType>(cmzn_selectionevent_get_change_summary(id));
	}

};

class Selectioncallback
{
friend class Selectionnotifier;
private:
	Selectioncallback(Selectioncallback&); // not implemented
	Selectioncallback& operator=(Selectioncallback&); // not implemented

	static void C_callback(cmzn_selectionevent_id selectionevent_id, void *callbackVoid)
	{
		Selectionevent selectionevent(cmzn_selectionevent_access(selectionevent_id));
		Selectioncallback *callback = reinterpret_cast<Selectioncallback *>(callbackVoid);
		(*callback)(selectionevent);
	}

	int set_C_callback(cmzn_selectionnotifier_id selectionnotifier_id)
	{
		return cmzn_selectionnotifier_set_callback(selectionnotifier_id, C_callback, static_cast<void*>(this));
	}

  virtual void operator()(const Selectionevent &selectionevent) = 0;

protected:
	Selectioncallback()
	{ }

public:
	virtual ~Selectioncallback()
	{ }
};

class Selectionnotifier
{
protected:
	cmzn_selectionnotifier_id id;

public:

	Selectionnotifier() : id(0)
	{  }

	// takes ownership of C handle, responsibility for destroying it
	explicit Selectionnotifier(cmzn_selectionnotifier_id in_selectionnotifier_id) :
		id(in_selectionnotifier_id)
	{  }

	Selectionnotifier(const Selectionnotifier& selectionNotifier) :
		id(cmzn_selectionnotifier_access(selectionNotifier.id))
	{  }

	Selectionnotifier& operator=(const Selectionnotifier& selectionNotifier)
	{
		cmzn_selectionnotifier_id temp_id = cmzn_selectionnotifier_access(selectionNotifier.id);
		if (0 != id)
		{
			cmzn_selectionnotifier_destroy(&id);
		}
		id = temp_id;
		return *this;
	}

	~Selectionnotifier()
	{
		if (0 != id)
		{
			cmzn_selectionnotifier_destroy(&id);
		}
	}

	bool isValid()
	{
		return (0 != id);
	}

	cmzn_selectionnotifier_id getId()
	{
		return id;
	}

	int setCallback(Selectioncallback& callback)
	{
		return callback.set_C_callback(id);
	}

	int clearCallback()
	{
		return cmzn_selectionnotifier_clear_callback(id);
	}
};

}  // namespace Zinc
}

#endif
