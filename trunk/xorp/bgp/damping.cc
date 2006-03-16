// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2006 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include <math.h>
#include <vector>

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "damping.hh"
#include "bgp.hh"

Damping::Damping(EventLoop& eventloop)
    :	 _eventloop(eventloop),
	 _damping(false),
	 _half_life(15),
	 _max_hold_down(60),
	 _cutoff(3000),
	 _tick(0)
{
    init();
}

void
Damping::set_damping(bool damping)
{
    _damping = damping;
    init();
}

bool
Damping::get_damping() const
{
    return _damping;
}

void
Damping::set_half_life(uint32_t half_life)
{
    _half_life = half_life;
    init();
}

void
Damping::set_max_hold_down(uint32_t max_hold_down)
{
    _max_hold_down = max_hold_down;
    init();
}

void
Damping::set_reuse(uint32_t reuse)
{
    _reuse = reuse;
    init();
}

void
Damping::set_cutoff(uint32_t cutoff)
{
    _cutoff = cutoff;
    init();
}

void
Damping::init()
{
    debug_msg("init\n");

    if (!_damping) {
	halt();
	return;
    }

    size_t array_size = _max_hold_down * 60;	// Into seconds.
    _decay.resize(array_size);

    double decay_1 = exp((1.0 / (_half_life * 60.0)) * log(1.0/2.0));
    double decay_i = decay_1;
    for (size_t i = 0; i < array_size; i++) {
	_decay[i] = static_cast<uint32_t>(decay_i * FIXED);
// 	printf("%d %d %f\n",i, _decay[i], decay_i);
	decay_i = pow(decay_1, static_cast<int>(i + 2));
    }

    // Start the timer to incement the tick
    _tick_tock = _eventloop.new_periodic(1000, callback(this, &Damping::tick));
}

void
Damping::halt()
{
    _tick_tock.unschedule();
}

bool
Damping::tick()
{
    debug_msg("tick\n");

    _tick++;    

    return true;
}

uint32_t
Damping::compute_merit(uint32_t last_time, uint32_t last_merit) const
{
    uint32_t tdiff = get_tick() - last_time;
    if (tdiff >= _max_hold_down * 60)
	return FIXED;
    else
	return (last_merit * _decay[tdiff]) / FIXED;
}

uint32_t
Damping::get_reuse_time(uint32_t merit) const
{
    uint32_t damp_time = (((merit / _reuse) - 1) * _half_life * 60);
    uint32_t max_time = _max_hold_down * 60; 

    return damp_time > max_time ? max_time : damp_time;
}
