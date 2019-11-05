#include "bpt.h"

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

void print_tree(int table_id) {
    Queue q;
    int i;
    pagenum_t tmp_num;
    buffer_t* header_page, * tmp;
    int64_t enter_key;

    q.front = 0;
    q.rear = -1;
    q.item_count = 0;

    header_page = get_buf(table_id, 0, 0);
    tmp_num = header_page->frame.header.root_page_number;

    unpin(header_page);
    
    // case: no root
    if (tmp_num == 0) {
        printf("empty tree\n");
        return;
    }

    // enqueue root number
    enqueue(&q, tmp_num);

    while (q.item_count != 0) {
        tmp_num = dequeue(&q);
        printf("(%ld) ", tmp_num);
        tmp = get_buf(table_id, tmp_num, 0);
        
        // case: leaf node
        if (tmp->frame.node.is_leaf) {
            for (i = 0; i < tmp->frame.node.number_of_keys; i++) {
                printf("%ld ", tmp->frame.node.key_values[i].key);
            }
            printf("| ");
        }
        else {
            enqueue(&q, tmp->frame.node.one_more_page_number);
            for (i = 0; i < tmp->frame.node.number_of_keys; i++) {
                printf("%ld ", tmp->frame.node.key_page_numbers[i].key);
                enqueue(&q, tmp->frame.node.key_page_numbers[i].page_number);
            }
            printf("| ");

            // case: last node in the level
            if (header_page->frame.header.root_page_number == tmp_num ||
                tmp->frame.node.key_page_numbers[0].key >= enter_key) {
                printf("\n");
                enter_key = tmp->frame.node.key_page_numbers[tmp->frame.node.number_of_keys - 1].key;
            }
        }
        unpin(tmp);
    }
    printf("\n");
}
