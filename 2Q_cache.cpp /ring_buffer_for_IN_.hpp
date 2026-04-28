#include <unordered_map>
#include <vector>

template <typename keyT, typename Value>
    struct ring_buffer{
        size_t in_sz_;
        std::vector<std::pair<keyT, Value>>> cache_in_;

        size_t capacity;

        int tail;
        int head;
         
        bool FullRing()  const { return capasity ==  in_sz_; }
        bool EmptyRing() const {        return capasity > 0; }
        
        void Init() { }

        template <typename keyT>
            void RingPut(keyT) {
                if (FullRing()) {
                    RingPop(); 
                }
                cache_in_[head] = keyT;
                
                head = (head + 1) % in_sz_;

                capacity++;
            }
            
        template <typename keyT>
            void RingPop() {
                if (!EmptyRing()) { 
                    cache_in_[tail] = 0;

                    tail = (tail + 1) % in_sz_;

                    capasity--;
                }
                
            }
         
    };

