#include "bpt.hpp"

void enqueue(Queue* q, pagenum_t data) {
    if(q->item_count != QUEUE_SIZE) {
        if(q->rear == QUEUE_SIZE - 1) {
            q->rear = -1;            
        }       

      q->pages[++q->rear] = data;
      q->item_count++;
   }
}

pagenum_t dequeue(Queue* q) {
    int data = q->pages[q->front++];
	
    if(q->front == QUEUE_SIZE) {
        q->front = 0;
    }
	
    q->item_count--;
    return data;  
}

int print_table(int table_id) {
    if (check_fd(table_id) == NOT_FOUND) return NOT_FOUND;
    Queue q;
    int i;
    pagenum_t tmp_num;
    buffer_t* header_page, * tmp;
    int64_t enter_key;

    int fd = open("print_table_result.txt", O_CREAT | O_NOFOLLOW | O_RDWR | O_TRUNC, 0666);

    q.front = 0;
    q.rear = -1;
    q.item_count = 0;

    header_page = get_buf(table_id, 0, 0);
    tmp_num = header_page->frame.header.root_page_number;

    unpin(header_page);
    
    // case: no root
    if (tmp_num == 0) {
        dprintf(fd, "empty tree\n");
        return 0;
    }

    // enqueue root number
    enqueue(&q, tmp_num);

    while (q.item_count != 0) {
        tmp_num = dequeue(&q);
        dprintf(fd, "(%lu) ", tmp_num);
        tmp = get_buf(table_id, tmp_num, 0);
        
        // case: leaf node
        if (tmp->frame.node.is_leaf) {
            for (i = 0; i < tmp->frame.node.number_of_keys; i++) {
                dprintf(fd, "%ld ", tmp->frame.node.key_values[i].key);
            }
            dprintf(fd, "| ");
        }
        else {
            enqueue(&q, tmp->frame.node.one_more_page_number);
            for (i = 0; i < tmp->frame.node.number_of_keys; i++) {
                dprintf(fd, "%ld ", tmp->frame.node.key_page_numbers[i].key);
                enqueue(&q, tmp->frame.node.key_page_numbers[i].page_number);
            }
            dprintf(fd, "| ");

            // case: last node in the level
            if (header_page->frame.header.root_page_number == tmp_num ||
                tmp->frame.node.key_page_numbers[0].key >= enter_key) {
                dprintf(fd, "\n");
                enter_key = tmp->frame.node.key_page_numbers[tmp->frame.node.number_of_keys - 1].key;
            }
        }
        unpin(tmp);
    }
    dprintf(fd, "\n");
    return 0;
}

void print_buf() {
    printf("buf info - mru: %d, lru: %d\n", buf_pool.mru, buf_pool.lru);
    int buf_num = buf_pool.mru;
    buffer_t* buf = buf_pool.buffers + buf_num;
    while (buf->prev != -1) {
        printf("buf_num: %d, table_id: %d, pagenum: %lu\n", buf_num, buf->table_id, buf->page_num);
        buf_num = buf->prev;
        buf = buf_pool.buffers + buf_num;
    }
    printf("buf_num: %d, table_id: %d, pagenum: %lu\n", buf_num, buf->table_id, buf->page_num);
}
