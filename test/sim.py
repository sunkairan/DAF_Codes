from __future__ import division

import os as _os

import random
from random import Random as _Random

from bats import Encoder, Decoder, Recoder
import time
from numpy import array


# Runtime configuration
T = 1
N = 1
M = 16
mode = 'packet'
hop = 2
LOSS_1 = 0.1
LOSS_2 = 0.1
overhead_suc_map = {}
overhead_fail_map = {}
packets = [(p+1)*1024 for p in range(100)]
# packets = [1024, 1024*2]
fractions = []


class ChannelInfo():
    def __init__(self, num_sent=0, num_received=0):
        self.num_sent = num_sent
        self.num_received = num_received


def _run_batch_mode(encoder, decoder, recoder):

    c1 = ChannelInfo()
    c2 = ChannelInfo()
    random_generator_1 = _Random()
    random_generator_2 = _Random()

    recoder_buffer = []

    #first_null = encoder.genPacket()
    #assert(first_null == '')

    while True:
        # channel 1 
        for i in xrange(M):
            packet = encoder.genPacket()
            assert(packet is not '')
            c1.num_sent += 1
            if random_generator_1.random() >= LOSS_1:
                recoder_buffer.append(packet)
                c1.num_received += 1

        # channel 2
        if len(recoder_buffer) is 0:
            continue

        recoder_data = ''.join(recoder_buffer)
        for i in xrange(M):
            recoded_packet = recoder.genPacket(recoder_data, len(recoder_buffer))
            c2.num_sent += 1
            if random_generator_2.random() >= LOSS_2:
                decoder.receivePacket(recoded_packet)
                c2.num_received += 1

                # bye bye
                if decoder.complete():
                    return decoder.buf, c1, c2

        # prepare to start a new batch
        packet = encoder.genPacket()
        assert(packet is '')
        del recoder_buffer[0:len(recoder_buffer)]


def _run_packet_mode(encoder, decoder, recoder):
    
    c1 = ChannelInfo()
    c2 = ChannelInfo()
    random_generator_1 = _Random()
    random_generator_2 = _Random()

    recoder_buffer = []
    dec_start = None
    dec_end = None
    
    #first_null = encoder.genPacket()
    #assert(first_null == '')

    while not decoder.complete():
        for x in xrange(M):
            packet = encoder.genPacket()
            assert(packet != '')
            c1.num_sent += 1
            
            # channel 1 loss
            if random_generator_1.random() > LOSS_1:
                recoder_buffer.append(packet)
                c1.num_received += 1

            if len(recoder_buffer) is 0:
                continue

            recoder_data = ''.join(recoder_buffer)
            recoded_packet = recoder.genPacket(recoder_data, len(recoder_buffer))
            c2.num_sent += 1

            # channel 2 loss
            if random_generator_2.random() > LOSS_2:
                
                decoder.receivePacket(recoded_packet)
                                
                c2.num_received += 1
                if decoder.complete():
                    break

        del recoder_buffer[0:len(recoder_buffer)]
        packet = encoder.genPacket()

        if not decoder.complete():
            assert(packet is '')

    return decoder.buf, c1, c2

def _run_one_hop(encoder, decoder):
    c1 = ChannelInfo()
    random_generator_1 = _Random()

    dec_start = None
    dec_end = None
    
    #first_null = encoder.genPacket()
    #assert(first_null == '')

    while not decoder.complete():
        for x in xrange(M):
            packet = encoder.genPacket()
            assert(packet != '')
            c1.num_sent += 1

            # channel 1 loss
            if random_generator_1.random() > LOSS_1:
                decoder.receivePacket(packet)
                c1.num_received += 1
                if decoder.complete():
                    break

        # generate a null packet
        packet = encoder.genPacket()
        if not decoder.complete():
            assert(packet is '')

    return decoder.buf, c1

def run_network(mode, src, dst, recoder):


    # each loop sends one batch from source to destination node.
    if mode is 'batch':
        return _run_batch_mode(src, dst, recoder)
    elif mode is 'packet':
        return _run_packet_mode(src, dst, recoder)
    else:
        print "mode error"
        return none

def run_two_hop(pkt_num):

    pkt_size = T
    ff_order = 8

    input_data = ''
    for x in xrange(pkt_num):
        input_data = input_data + pkt_size * chr(random.randint(0, 255))

    # define network's source and destination
    src = Encoder(M, pkt_num, pkt_size, input_data)
    dst = Decoder(M, pkt_num, pkt_size)
    recoder = Recoder(M, pkt_size)

    start = time.clock()    
    output_data, c1, c2 = run_network(mode, src, dst, recoder)
    end = time.clock()

    #print "output_data's length: ", len(output_data)

    # for i in xrange(pkt_num * pkt_size):
    #     if input_data[i] != output_data[i]:
    #         print "#"*30 + " memory coding error " + "#"*30
    #         break

    return end - start, ((c1.num_sent, c1.num_received), (c2.num_sent, c2.num_received), dst.num_inact)

def run_one_hop(pkt_num):
    pkt_size = T
    ff_order = 8

    input_data = ''
    for x in xrange(pkt_num):
        input_data = input_data + pkt_size * chr(random.randint(0, 255))

    # define network's source and destination
    src = Encoder(M, pkt_num, pkt_size, input_data)
    dst = Decoder(M, pkt_num, pkt_size)
    

    start = time.clock()    
    output_data, c1 = _run_one_hop(src, dst)
    end = time.clock()

    # print "output_data's length: ", len(output_data)

    for i in xrange(pkt_num * pkt_size):
        if input_data[i] != output_data[i]:
            print "#"*30 + " memory coding error " + "#"*30
            break

    return end - start, c1.num_sent, c1.num_received, dst.num_inact


def get_overhead(pkt_num, received_packets):
    overhead =  received_packets - pkt_num
    fraction = float(overhead) / float(pkt_num)
    return overhead, fraction



def run():
    global overhead_suc_map
    global overhead_fail_map
    global packets
    global fractions

    base_time = None
    base_time_dec = None
    # generate overhead-success map
    for src in packets:
        o_s_map = {}

        for i in xrange(N):
            pkt_num = src
            print "=" * 30
            sent_packets = None
            received_packets = None

            if hop is 1:
                t, c1_sent_packets, c1_received_packets, num_inact = run_one_hop(pkt_num)

                if base_time is None:
                    base_time = t
                
                sent_packets = c1_sent_packets
                received_packets = c1_received_packets
                print 'packet_num: %d, times: %f' % (src, src/packets[0])
                print 'time: %f, times: %f' % (t, t/base_time)
                print 'num_inact = %d' % num_inact
                print 'c1_sent_packets = %d, c1_received_packets = %d' % (c1_sent_packets, c1_received_packets)


            elif hop is 2:
                t, ((c1_sent_packets, c1_received_packets), (c2_sent_packets, c2_received_packets), num_inact) = run_two_hop(pkt_num)
                
                if base_time is None:
                    base_time = t

                sent_packets = c1_sent_packets
                received_packets = c2_received_packets
                print 'packet_num: %d, times: %f' % (src, src/packets[0])
                print 'time: %f, times: %f' % (t, t/base_time)
                print 'num_inact = %d' % num_inact
                print 'c1_sent_packets = %d, c1_received_packets = %d, c2_sent_packets = %d, c2_received_packet = %d' % (c1_sent_packets,
                                                                                                                         c1_received_packets,
                                                                                                                         c2_sent_packets,
                                                                                                                         c2_received_packets)

            else:
                print 'hop num is invalid'
                return

            overhead, fraction = get_overhead(pkt_num, received_packets)
            coding_rate = float(pkt_num) / float(sent_packets)
            print 'raw_packets = %d, sent_packets = %d, received_packets = %d, overhead = %d, fraction = %f, coding_rate = %f' % (pkt_num,
                                                                                                                                  sent_packets,
                                                                                                                                  received_packets,
                                                                                                                                  overhead,
                                                                                                                                  fraction,
                                                                                                                                  coding_rate)
            fractions.append(fraction)

            if overhead_suc_map.has_key((src, overhead)):
                o_s_map[overhead] += 1
            else:
                o_s_map[overhead] = 1

        overhead_suc_map[src] = o_s_map

    # generate overhead-fail map
    for file_name in overhead_suc_map.keys():

        suc_result_list = overhead_suc_map[file_name].items()

        #print '-'*20 + str(file_name) + '-'*20


        suc_result_list =  sorted(suc_result_list, key=lambda o_s_pair: o_s_pair[0])

        pre_suc = 0
        o_f_map = {}
        for i in xrange(len(suc_result_list)):

            fails_at_this = N - pre_suc - suc_result_list[i][1]

            o_f_map[suc_result_list[i][0]] = fails_at_this / float(N)

            pre_suc += suc_result_list[i][1]
            
        overhead_fail_map[file_name] = o_f_map
        
def print_result():
    print fractions
    f_result = array(fractions)
    print 'average overhead: %f, std: %f' % (f_result.mean(), f_result.std())

    # for packet_length in overhead_fail_map:
    #     print '='*20 + ' ' + str(packet_length) + ' ' + '='*20

    #     curve = overhead_fail_map[packet_length]
    #     for overhead in curve:
    #         fail_prob = curve[overhead]
    #         print str(overhead) + ':' + '\t'*3 + str(fail_prob)

        

if __name__ == '__main__':
    run()
    print_result()
    # print overhead_suc_map
